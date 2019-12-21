import re
import sys
import json
import traceback

import gdb


class statistic(object):
    def __init__(self):
        self.executed = 0
        self.errors = 0
        self.warnings = 0
        self.children = []
        self.tests = []
        self.cancelled = False
        self.parent = None
        self.name = "__main__"

    def __iadd__(self, rhs):
        self.executed += rhs.executed
        self.errors += rhs.errors
        self.warnings += rhs.warnings
        return self

    def cancel(self, args, frame, report):
        gdb.execute("set var __metal_critical = 0")
        fr = frame

        if fr.function().name == "main":
            self.cancelled = True
            gdb.write("{} canceling main: {{executed: {}, warnings: {}, errors: {}}}\n".format(
                                loc_str(args, frame), self.executed, self.warnings, self.errors))
            report(args, frame)
            gdb.execute("return 1")
        try:

            while fr != None:
                is_metal_call = fr.function().name.startswith("__metal_call(") or fr.function().name == "__metal_call"
                is_main = fr.function().name == "main"

                if is_metal_call:
                    stat_id = str_arg(args, frame, 0)
                    fr.select()
                    self.cancelled = True
                    gdb.write("{} canceling test case [{}]: {{executed: {}, warnings: {}, errors: {}}}\n".format(
                        loc_str(args, frame), self.name, self.executed, self.warnings, self.errors))
                    gdb.execute("return")
                    report(args, frame)
                    return

                if is_main:
                    fr.select()
                    self.cancelled = True
                    gdb.write("{} canceling to main: {{executed: {}, warnings: {}, errors: {}}}\n".format(
                                    loc_str(args, frame), self.executed, self.warnings, self.errors))
                    gdb.execute("return")
                    report(args, frame)
                fr = fr.older()
        except gdb.error as e:
            gdb.write("PANIC!!! Error cancelling, couldn't find frame to cancel to: {}\n".format(e))
            try:
                gdb.write("Last frame found was {}\n".format(fr.function().name))
            except: pass
            gdb.write("Terminating gdb\n")
            gdb.post_event(lambda: gdb.execute("q -1"))

    def append_test(self, ck, args, frame, report):
        self.tests.append(ck)
        if ck["condition"] is False and ck["critical"] is True:
            self.cancel(args, frame, report)

    def toDict(self):
        return {
                "summary": {
                    "executed": self.executed,
                    "warnings": self.warnings,
                    "errors": self.errors
                },
                "cancelled": self.cancelled,
                "children": [ch.toDict() for ch in self.children],
                "tests":    self.tests,
            }


class case(statistic):
    def __init__(self, name):
        super(case, self).__init__()
        self.name = name
        self.type = "case"

    def toDict(self):
        par = super(case, self).toDict()
        par["name"] = self.name
        par["type"] = self.type
        return par

class ranged_test(statistic):
    def __init__(self):
        super(ranged_test, self).__init__()
        self.type = "range"
        self.index = 0
        self.critical_fail = False

    def cancel(self, args, frame, rep):
        self.critical_fail = True

    def exit(self, args, frame, report):
        if self.critical_fail:
            statistic.cancel(self, args, frame, report)

    def toDict(self):
        par = super(ranged_test, self).toDict()
        par["length"] = len(self.tests)
        par["type"] = self.type
        par["critical_fail"] = self.critical_fail
        return par


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


class Level:
    assertion = 0
    expect = 1


def descr(lvl):
    if lvl == Level.assertion: return "assertion"
    elif lvl == Level.expect : return "expectation"
    else: return ""

def str_arg(args, frame, idx): return str(args[idx + 4].value(frame).string())


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
        return int(str(gdb.lookup_global_symbol("__metal_critical").value())) != 0
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
            if isinstance(mem, memoryview):
                for b in mem.tobytes():
                    res += bin(b)[2:]
            else:
                for b in mem:
                    res += bin(ord(b))[2:]


            return "0b" + res
        sym = gdb.parse_and_eval(name)

        return str(sym)


class metal_test_backend(gdb.Breakpoint):
    def __init__(self):
        gdb.Breakpoint.__init__(self, "__metal_impl")

        self.summary = statistic()
        self.current_scope = self.summary
        self.case = None
        self.range = None

    def stop(self):
        try:
            fr = gdb.selected_frame()
            args = [arg for arg in fr.block() if arg.is_argument]
            oper = str(args[1].value(fr))[len("__metal_oper_"):]

            if oper == "exec":
                self.exec_(args, fr)
            else:
                getattr(self, oper)(args, fr)

            return False
        except gdb.error as e:
            gdb.write("Error in metal-unit.py: {}".format(e), gdb.STDERR)
            traceback.print_exc()
            raise e

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

        gdb.write("{} {} test case [{}]: {{executed: {}, warnings: {}, errors: {}}}\n".format(
            loc_str(args, frame), "canceled" if self.case.cancelled else "exited",
            self.case.name, self.case.executed, self.case.warnings, self.case.errors))

        par = self.case.parent
        delattr(self.case, "parent")
        if isinstance(par, case):
            self.case = par

    def enter_ranged(self, args, frame):
        if isinstance(self.current_scope, ranged_test):
            gdb.write("{} critical error: Twice enter into ranged test, check your test!!\n".format(loc_str(args, frame)))
            return
        ck, prefix, cs = self.__check(args, frame)


        cond = condition(args, frame)
        if cond:
            descr = str_arg(args, frame, 0)
            lhs = str_arg(args, frame, 1)
            rhs = str_arg(args, frame, 2)
            lhs_val = print_from_frame(args, frame, False, 1, lhs)
            rhs_val = print_from_frame(args, frame, False, 1, rhs)
            gdb.write("{} error entering ranged test[{}] with mismatch: {} != {}".format(prefix, descr,  lhs_val, rhs_val))
            return


        descr = str_arg(args, frame, 0)
        gdb.write("{} entering ranged test[{}]\n".format(loc_str(args, frame), descr))

        self.range = ranged_test()
        self.range.parent = self.current_scope
        self.current_scope.children.append(self.range)
        self.current_scope = self.range

    def exit_ranged(self, args, frame):
        if self.range is None:
            return

        self.range.exit(args, frame, self.report)

        descr = str_arg(args, frame, 0)
        gdb.write("{} {}} ranged test [{}]: {{executed: {}, warnings: {}, errors: {}}}\n".format(
            loc_str(args, frame), "canceled" if self.case.cancelled else "exited", descr, self.case.executed, self.case.warnings, self.case.errors))

        self.range.parent.executed += 1
        if self.range.errors > 0:
            self.range.parent.errors += 1

        if self.range.warnings > 0:
            self.range.parent.warnings += 1

        self.range.parent += self.range
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
        ck, prefix, cs = self.__check(args, frame)
        message = str_arg(args, frame, 0)
        gdb.write("{} [message]: {}\n".format(prefix, message))
        ck["message"] = message
        ck["type"] = "message"
        cs.append_test(ck, args, frame, self.report_canceled)

    def plain(self, args, frame):
        ck, prefix, cs = self.__check(args, frame)
        message = str_arg(args, frame, 0)
        gdb.write("{} [expression]: {}\n".format(prefix, message))
        ck["message"] = message
        ck["type"] = "plain"
        cs.append_test(ck, args, frame, self.report_canceled)

    def predicate(self, args, frame):
        ck,prefix, current_scope = self.__check(args, frame)
        name = str_arg(args, frame, 0)
        args_ = str_arg(args, frame, 1)
        gdb.write("{} [predicate]: {}({})\n".format(prefix, name, args_))
        ck["name"] = name
        ck["args"] = args_
        ck["type"] = "plain"
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def equal(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
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
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def not_equal(self, args, frame):
        ck, prefix ,current_scope = self.__check(args, frame)
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
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def close(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        tolerance = str_arg(args, frame, 2)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)
        tolerance_val = print_from_frame(args, frame, bw, 1, tolerance)

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
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def close_rel(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        tolerance = str_arg(args, frame, 2)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)
        tolerance_val = print_from_frame(args, frame, bw, 1, tolerance)

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
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def close_perc(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
        lhs = str_arg(args, frame, 0)
        rhs = str_arg(args, frame, 1)
        tolerance = str_arg(args, frame, 2)
        bw = bitwise(args, frame)
        bw_s = "bitwise " if bw else ""

        lhs_val = print_from_frame(args, frame, bw, 1, lhs)
        rhs_val = print_from_frame(args, frame, bw, 1, rhs)
        tolerance_val = print_from_frame(args, frame, bw, 1, tolerance)
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
        current_scope.append_test(ck, args, frame, self.report_canceled)


    def ge(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
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
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def greater(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
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
        current_scope.append_test(ck, args, frame, self.report_canceled)


    def le(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
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
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def lesser(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
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
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def exception(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
        expected = str_arg(args, frame, 0)
        got = str_arg(args, frame, 1)
        gdb.write("{} throw exception: [{}] got [{}]\n".format(prefix, expected, got))
        ck["type"] = "exception"
        ck["got"] = got
        ck["expected"] = expected
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def any_exception(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
        gdb.write("{} throw any exception.\n".format(prefix))
        ck["type"] = "any_exception"
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def no_exception(self, args, frame):
        ck, prefix,current_scope = self.__check(args, frame)
        gdb.write("{} throw no exception.\n".format(prefix))
        ck["type"] = "no_exception"
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def no_exec(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
        gdb.write("{} do not execute.\n".format(prefix))
        ck["type"] = "no_execute_check"
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def exec_(self, args, frame):
        ck, prefix, current_scope = self.__check(args, frame)
        gdb.write("{} do execute.\n".format(prefix))
        ck["type"] = "execute_check"
        current_scope.append_test(ck, args, frame, self.report_canceled)

    def report(self, args, frame):

        gdb.write("full test report: {{executed: {}, warnings: {}, errors: {}}}\n".format(
            self.summary.executed, self.summary.warnings, self.summary.errors))

        if selectJsonSink.value:
            if selectJsonSink.sink:
                selectJsonSink.sink.write(json.dumps(self.summary.toDict()))
            else:
                with open(selectJsonSink.value, "w") as fl:
                    fl.write(json.dumps(self.summary.toDict()))

    def report_canceled(self, args, frame):
        self.summary += self.case
        self.report(args, frame)

    def __check(self, args, frame):

        crit = is_critical(args, frame)
        cs = self.current_scope
        self.current_scope.executed += 1
        cond = condition(args, frame)
        lvl = level(args, frame)
        if not cond:
            if lvl == Level.expect:
                self.current_scope.warnings += 1
            else:
                self.current_scope.errors += 1
            if crit:
                self.current_scope = self.current_scope.parent

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

        return res, "{}({}){} {} {}".format(f, l, "critical " if crit else "", lvl_descr, "succeeded" if cond else "failed"), cs

mtb = metal_test_backend()

gdb.execute("set pagination off")
