import re
import sys
import json
from typing import List

import gdb


class statistic:
    def __init__(self):
        self.executed = 0
        self.errors = 0
        self.warnings = 0
        self.children = []
        self.tests = []

    def __iadd__(self, rhs):
        self.executed += rhs.executed
        self.errors += rhs.errors
        self.warnings += rhs.warnings
        return self

    def cancel(self, args, frame):
        print("Cancel")

    def toJSON(self):
        return json.dumps("foo\n\n")


class case(statistic):
    def __init__(self, name):
        super(case, self).__init__()
        self.name = name
        self.type = "case"


class ranged_test(statistic):
    def __init__(self):
        super(ranged_test, self).__init__()
        self.type = "range"
        self.index = 0

    def enter(self, args, frame):
        pass

    def exit(self, args, frame):
        pass

    def cancel(self, args, frame):
        pass


class DisableExitCode(gdb.Parameter):
    def __init__(self):
        super(DisableExitCode, self).__init__("metal-test-exit-code",
                                              gdb.COMMAND_DATA,
                                              gdb.PARAM_BOOLEAN)
        self.value = True

    set_doc = '''Set exit-code propagation.'''
    show_doc = '''This parameter enables assignment of the exit-code on test exit.'''


disableExitCode = DisableExitCode()


class SelectJsonSink(gdb.Parameter):
    def __init__(self):
        super(SelectJsonSink, self).__init__("metal-test-json-file",
                                             gdb.COMMAND_DATA,
                                             gdb.PARAM_OPTIONAL_FILENAME)
        self.value = "stdout"
        self.sink = sys.stdout

    def get_set_string(self):
        if self.value is None:
            self.sink = None
        elif self.value == 'stdout':
            self.sink = sys.stdout
        elif self.value == 'stderr':
            self.sink = sys.stderr
        return self.value

    set_doc = '''Set output file.'''
    show_doc = '''This sets the test data output sink.'''


selectJsonSink = SelectJsonSink()

from enum import Enum


class Level(Enum):
    assertion = 0
    expect = 1


def descr(lvl):
    if lvl == Level.assertion: return "assertion"
    elif lvl == Level.expect : return "expectation"
    else: return ""

def str_arg(args: List[gdb.Symbol], frame, idx): return str(args[idx + 4].value(frame).string())


def level(args, frame):
    return {
        "__metal_level_assert": Level.assertion,
        "__metal_level_expect": Level.expect
    }[str(args[0].value(frame))]


def condition(args, frame): return int(args[2].value(frame)) != 0


def bitwise(args, frame): return int(args[3].value(frame)) != 0


def file(args, frame): return str(args[7].value(frame).string())


def line(args, frame): return int(args[8].value(frame))


def loc_str(args, frame): return "{}({})".format(file(args, frame).replace('\\\\', '\\'), line(args, frame))


def loc_tup(args, frame):
    file_ = file(args, frame)
    line_ = line(args, frame)
    return file_, line_, "{}({})".format(file_.replace('\\\\', '\\'), line_)


has_no_critical = False


def is_critical(args, frame):
    global has_no_critical
    if has_no_critical:
        return False
    try:
        return int(str(gdb.lookup_global_symbol("__metal_critical"))) != 0
    except:
        has_no_critical = True
        return False

class frameSelector:

    def __init__(self, frame, idx = 1):
        self.frame = frame
        self.index = idx

    def __enter__(self):
        fr = self.frame
        for i in range(0, self.index):
            fr = fr.older()
        fr.select()
        return self

    def __exit__(self, x, y, z):
        self.frame.select()


def print_from_frame(args, frame, bw, frame_nr, name):

    with frameSelector(frame, frame_nr):
        is_var = re.match("[^A-Za-z_]\\w*", name) == None

        if bw and is_var:
            sz = int(str(gdb.parse_and_eval("sizeof({})".format(name))), 16)
            addr = int(str(gdb.parse_and_eval("&" + name)).split(' ')[0], 16)

            mem = gdb.selected_inferior().read_memory(addr, sz)
            res = ""
            for b in mem.tobytes():
                res += bin(b)[2:]

            return "0b" + res
        sym = gdb.parse_and_eval(name)

        return str(sym)


class metal_test_backend(gdb.Breakpoint):
    def __init__(self):
        gdb.Breakpoint.__init__(self, "__metal_impl")

        self.summary = statistic()
        self.free_tests = statistic()
        self.current_scope = self.free_tests
        self.case = None
        self.range = None

    def stop(self):
        fr = gdb.selected_frame()
        args = [arg for arg in fr.block() if arg.is_argument]
        oper = str(args[1].value(fr))[len("__metal_oper_"):]

        try:
            getattr(self, oper)(args, fr)
        except Exception as e:
            gdb.write("Internal error {}\n".format(e), gdb.STDERR)

        gdb.post_event(lambda: gdb.execute("continue"))

    def enter_case(self, args, frame):
        id = str_arg(args, frame, 0)
        self.case = case(id)
        self.current_scope.children.append(self.case)
        self.case.parent = self.current_scope
        self.current_scope = self.case

        gdb.write("{} entering test case [{}]\n".format(loc_str(args, frame), id))

    def exit_case(self, args, frame):
        case_id = str_arg(args, frame, 0)

        self.summary += self.case
        self.current_scope = self.case.parent
        gdb.write("{} exiting test case [{}]: {{executed: {}, warnings: {}, errors: {}}}\n".format(
            loc_str(args, frame), case_id, self.case.executed, self.case.warnings, self.case.errors))

        par = self.case.parent
        delattr(self.case, "parent")
        if isinstance(par, case):
            self.case = par

    def enter_ranged(self, args, frame):
        if isinstance(self.current_scope, ranged_test):
            gdb.write("{} critical error: Twice enter into ranged test, check your test!!\n".format(loc_str(args, frame)))
            return

        self.range = ranged_test()
        self.range.parent = self.current_scope
        self.current_scope.children.append(self.case)
        self.current_scope = self.range

        self.range.enter(args, frame)

    def exit_ranged(self, args, frame):

        self.range.exit(args, frame)

        self.summary += self.range
        self.current_scope = self.range.parent

        par = self.range.parent
        delattr(self.range, "parent")
        if isinstance(par, case):
            self.case = par
        self.range = None

    def log(self, args, frame):
        msg = str_arg(args, frame, 0)
        f, l, str_ = loc_tup(args, frame)
        gdb.write("{} log: {}\n".format(str_, str_arg(args, frame, 0)))
        self.current_scope.tests.append({"type": "message", "file": f, "line": l, "message": msg})

    def checkpoint(self, args, frame):
        f, l, str_ = loc_tup(args, frame)
        gdb.write("{} checkpoint\n".format(str_))
        self.current_scope.tests.append({"type": "checkpoint", "file": f, "line": l})

    def message(self, args, frame):
        ck, prefix = self.__check(args, frame)
        message = str_arg(args, frame, 0)
        gdb.write("{} [message]: {}\n".format(prefix, message))
        ck["message"] = message
        ck["type"] = "message"
        self.current_scope.tests.append(ck)

    def plain(self, args, frame):
        ck, prefix = self.__check(args, frame)
        message = str_arg(args, frame, 0)
        gdb.write("{} [expression]: {}\n".format(prefix, message))
        ck["message"] = message
        ck["type"] = "plain"
        self.current_scope.tests.append(ck)

    def predicate(self, args, frame):
        ck,prefix = self.__check(args, frame)
        name = str_arg(args, frame, 0)
        args_ = str_arg(args, frame, 1)
        gdb.write("{} [predicate]: {}({})\n".format(prefix, name, args_))
        ck["name"] = name
        ck["args"] = args_
        ck["type"] = "plain"
        self.current_scope.tests.append(ck)

    def equal(self, args, frame):
        ck, prefix = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)

        descr = "**range**[{}]".format(len(self.range.tests)) if self.range else "{} == {}".format(lhs, rhs)

        gdb.write("{} [{}equality]: {}; [{} == {}]\n".format(prefix, bw_s, descr, lhs_val, rhs_val))
        ck["type"] = "equal"
        ck["bitwise"] = bw
        ck["lhs"] = lhs
        ck["rhs"] = rhs
        ck["lhs_val"] = lhs_val
        ck["rhs_val"] = rhs_val
        self.current_scope.tests.append(ck)

    def not_equal(self, args, frame):
        ck, prefix = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)

        descr = "**range**[{}]".format(len(self.range.tests)) if self.range else "{} != {}".format(lhs, rhs)

        gdb.write("{} [{}equality]: {}; [{} != {}]\n".format(prefix, bw_s, descr, lhs_val, rhs_val))
        ck["type"] = "not_equal"
        ck["bitwise"] = bw
        ck["lhs"] = lhs
        ck["rhs"] = rhs
        ck["lhs_val"] = lhs_val
        ck["rhs_val"] = rhs_val
        self.current_scope.tests.append(ck)

    def close(self, args, frame):
        ck, prefix = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        tolerance = str_arg(args, frame, 2)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)
        tolerance_val = print_from_frame(args, frame, bw, 1, rhs)

        descr = "**range**[{}]".format(len(self.range.tests)) if self.range else "{} == {} +/- {}".format(lhs, rhs, tolerance)

        gdb.write("{} [{}equality]: {}; [{} != {} +/- {}]\n".format(prefix, bw_s, descr, lhs_val, rhs_val, tolerance_val))
        ck["type"] = "close"
        ck["bitwise"] = bw
        ck["lhs"] = lhs
        ck["rhs"] = rhs
        ck["tolerance"] = tolerance
        ck["lhs_val"] = lhs_val
        ck["rhs_val"] = rhs_val
        ck["tolerance_vale"] = tolerance_val
        self.current_scope.tests.append(ck)

    def close_rel(self, args, frame):
        ck, prefix = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        tolerance = str_arg(args, frame, 2)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)
        tolerance_val = print_from_frame(args, frame, bw, 1, rhs)

        descr = "**range**[{}]".format(len(self.range.tests)) if self.range else "{} == {} +/- {}~".format(lhs, rhs, tolerance)

        gdb.write("{} [{}equality]: {}; [{} != {} +/- {}~]\n".format(prefix, bw_s, descr, lhs_val, rhs_val, tolerance_val))
        ck["type"] = "close_rel"
        ck["bitwise"] = bw
        ck["lhs"] = lhs
        ck["rhs"] = rhs
        ck["tolerance"] = tolerance
        ck["lhs_val"] = lhs_val
        ck["rhs_val"] = rhs_val
        ck["tolerance_vale"] = tolerance_val
        self.current_scope.tests.append(ck)

    def close_perc(self, args, frame):
        ck, prefix = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        tolerance = str_arg(args, frame, 2)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)
        tolerance_val = print_from_frame(args, frame, bw, 1, rhs)
        descr = "**range**[{}]".format(len(self.range.tests)) if self.range else "{} == {} +/- {}%".format(lhs, rhs, tolerance)
        gdb.write("{} [{}equality]: {}; [{} != {} +/- {}%]\n".format(prefix, bw_s, descr, lhs_val, rhs_val, tolerance_val))
        ck["type"] = "close_per"
        ck["bitwise"] = bw
        ck["lhs"] = lhs
        ck["rhs"] = rhs
        ck["tolerance"] = tolerance
        ck["lhs_val"] = lhs_val
        ck["rhs_val"] = rhs_val
        ck["tolerance_vale"] = tolerance_val
        self.current_scope.tests.append(ck)


    def ge(self, args, frame):
        ck, prefix = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)

        descr = "**range**[{}]".format(len(self.range.tests)) if self.range else "{} >= {}".format(lhs, rhs)

        gdb.write("{} [{}comparison]: {}; [{} >= {}]\n".format(prefix, bw_s, descr, lhs_val, rhs_val))
        ck["type"] = "ge"
        ck["bitwise"] = bw
        ck["lhs"] = lhs
        ck["rhs"] = rhs
        ck["lhs_val"] = lhs_val
        ck["rhs_val"] = rhs_val
        self.current_scope.tests.append(ck)

    def greater(self, args, frame):
        ck, prefix = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)

        descr = "**range**[{}]".format(len(self.range.tests)) if self.range else "{} > {}".format(lhs, rhs)

        gdb.write("{} [{}comparison]: {}; [{} > {}]\n".format(prefix, bw_s, descr, lhs_val, rhs_val))
        ck["type"] = "ge"
        ck["bitwise"] = bw
        ck["lhs"] = lhs
        ck["rhs"] = rhs
        ck["lhs_val"] = lhs_val
        ck["rhs_val"] = rhs_val
        self.current_scope.tests.append(ck)


    def le(self, args, frame):
        ck, prefix = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)

        descr = "**range**[{}]".format(len(self.range.tests)) if self.range else "{} <= {}".format(lhs, rhs)

        gdb.write("{} [{}comparison]: {}; [{} <= {}]\n".format(prefix, bw_s, descr, lhs_val, rhs_val))
        ck["type"] = "le"
        ck["bitwise"] = bw
        ck["lhs"] = lhs
        ck["rhs"] = rhs
        ck["lhs_val"] = lhs_val
        ck["rhs_val"] = rhs_val
        self.current_scope.tests.append(ck)

    def lesser(self, args, frame):
        ck, prefix = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)

        descr = "**range**[{}]".format(len(self.range.tests)) if self.range else "{} < {}".format(lhs, rhs)

        gdb.write("{} [{}comparison]: {}; [{} < {}]\n".format(prefix, bw_s, descr, lhs_val, rhs_val))
        ck["type"] = "lesser"
        ck["bitwise"] = bw
        ck["lhs"] = lhs
        ck["rhs"] = rhs
        ck["lhs_val"] = lhs_val
        ck["rhs_val"] = rhs_val
        self.current_scope.tests.append(ck)

    def exception(self, args, frame):
        ck, prefix = self.__check(args, frame)
        expected = str_arg(args, frame, 0)
        got = str_arg(args, frame, 1)
        gdb.write("{} throw exception: [{}] got [{}]\n".format(prefix, expected, got))
        ck["type"] = "exception"
        ck["got"] = got
        ck["expected"] = expected
        self.current_scope.tests.append(ck)

    def any_exception(self, args, frame):
        ck, prefix = self.__check(args, frame)
        gdb.write("{} throw any exception.\n".format(prefix))
        ck["type"] = "any_exception"
        self.current_scope.tests.append(ck)

    def no_exception(self, args, frame):
        ck, prefix = self.__check(args, frame)
        gdb.write("{} throw no exception.\n".format(prefix))
        ck["type"] = "no_exception"
        self.current_scope.tests.append(ck)

    def no_exec(self, args, frame):
        ck, prefix = self.__check(args, frame)
        gdb.write("{} do not execute.\n".format(prefix))
        ck["type"] = "no_execute_check"
        self.current_scope.tests.append(ck)

    def exec(self, args, frame):
        ck, prefix = self.__check(args, frame)
        gdb.write("{} do execute.\n".format(prefix))
        ck["type"] = "execute_check"
        self.current_scope.tests.append(ck)

    def report(self, args, frame):
        if self.free_tests.executed > 0:
            gdb.write("free tests: {{executed: {}, warnings: {}, errors: {}}}\n".format(
                self.free_tests.executed, self.free_tests.warnings, self.free_tests.errors))

        gdb.write("full test report: {{executed: {}, warnings: {}, errors: {}}}\n".format(
            self.summary.executed, self.summary.warnings, self.summary.errors))

        if selectJsonSink.value:
            if selectJsonSink.sink:
                selectJsonSink.sink.write(self.summary.toJSON())
            else:
                with open(selectJsonSink.value, "w") as fl:
                    fl.write(self.summary.toJSON())

    def __check(self, args, frame):

        crit = is_critical(args, frame)

        self.current_scope.executed += 1
        cond = condition(args ,frame)
        lvl = level(args, frame)
        if not cond:
            if lvl == Level.expect:
                self.current_scope.warnings += 1
            else:
                self.current_scope.errors += 1
            if crit:
                self.current_scope.cancel(args, frame)
        f = file(args, frame)
        l = line(args, frame)
        crit = is_critical(args, frame)
        lvl_descr = descr(lvl)
        res = {
            "file": f,
            "line": l,
            "condition": cond,
            "lvl": lvl_descr,
            "critical": crit
        }
        if self.range:
            res["index"] = len(self.range.tests)
            
        return res, "{}({}){} {} {}".format(f, l, "critical " if crit else "", lvl_descr, "succeeded" if cond else "failed");

mtb = metal_test_backend()

gdb.execute("set pagination off")
