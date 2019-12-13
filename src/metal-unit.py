import sys
import gdb

def x(): print ("bla")

gdb.events.cont.connect()
gdb.write("foo, ", gdb.STDOUT)

class DisableExitCode(gdb.Parameter):
    def __init__ (self):
        super (DisableExitCode, self).__init__ ("metal-test-no-exit-code",
                                             gdb.COMMAND_DATA,
                                             gdb.PARAM_BOOLEAN)
        self.value = False

    set_doc = '''Disabled exit-code propagation.'''
    show_doc = '''This parameter disables assignment of the exit-code on test exit.'''

disableExitCode = DisableExitCode()

class SelectTestSink(gdb.Parameter):
    def __init__ (self):
        super (SelectTestSink, self).__init__ ("metal-test-sink",
                                                gdb.COMMAND_DATA,
                                                gdb.PARAM_OPTIONAL_FILENAME)
        self.value = "stdout"
        self.sink = sys.stdout

    def get_set_string(self):
        if self.value is None:
            self.sink = sys.stdout
        elif self.value == 'stdout':
            self.sink = sys.stdout
        elif self.value == 'stderr':
            self.sink = sys.stderr
        else:
            self.sink = open(str(self.value), 'w')
        return self.value

    set_doc = '''Set output file.'''
    show_doc = '''This sets the test data output sink.'''

selectTestSink = SelectTestSink()


class SelectTestFormat(gdb.Parameter):
    def __init__ (self):
        super (SelectTestFormat, self).__init__ ("metal-test-format",
                                                 gdb.COMMAND_DATA,
                                                 gdb.PARAM_ENUM, ["hrf", "json"])
        self.value = 'hrf'
        self.sink = hrf_sink()

    def get_set_string(self):
        if self.value == 'hrf':
            self.sink = hrf_sink()
            return "Set format to human-readable format"
        elif self.value == 'json':
            self.sink = json_sink()
            return "Set format to JSON"

    set_doc = '''Set output file.'''
    show_doc = '''This sets the test data output sink.'''

selectTestFormat = SelectTestFormat()


from enum import Enum
class Level(Enum):
    assertion = 0
    expect = 1


def str_arg(args, frame, idx): return str(args[idx+4].value(frame))

def level(args, frame):
    return {
        "__metal_level_assert": Level.assertion,
        "__metal_level_expect": Level.expect
    } [str(args[0].value(frame))]

def condition(args, frame): return int(args[2].value(frame)) != 0
def bitwise  (args, frame): return int(args[3].value(frame)) != 0
def file     (args, frame): return str(args[7].value(frame))
def line     (args, frame): return int(args[8].value(frame))


has_no_critical = False

def is_critical(args, frame):
    global has_no_critical
    if has_no_critical:
        return False
    try:
        return int(gdb.lookup_global_symbol("__metal_critical")) != 0
    except:
        has_no_critical = True
        return False

class statistics:
    def __init__(self):
        self.executed = 0
        self.errors   = 0
        self.warnings = 0

    def __iadd__(lhs, rhs):
        lhs.executed += rhs.executed
        lhs.errors   += rhs.errors
        lhs.warnings += rhs.warnings

class error_handler(statistics):
    def __init__(self):
        super(error_handler, self).__init__(self)

    def cancel(self, args, frame):


    def check(self, args, frame, func, *more_args):
        self.execute += 1
        lvl = level(args, frame)
        cond = condition(args, frame)
        crit = is_critical(args, frame)

        getattr(selectTestFormat.sink, func)(file(args, frame), line(args, frame), cond, lvl, crit, -1, *more_args)

        if cond:
            if lvl == Level.assertion:
                self.errors += 1
                if crit:
                    self.set_error(args, frame)
                    self.cancel(args, frame)
            else:
                self.warnings += 1
                if crit: self.canel(args, frame)


class free_t: pass
class range_t: pass
class case_t: pass





class free_t(error_handler):
    pass

class hrf_sink:
    pass

class json_sink:
    pass


class metal_test_backend(gdb.Breakpoint):
    def __init__(self):
        gdb.Breakpoint.__init__(self, "__metal_impl")


    def stop(self):
        fr = gdb.selected_frame()
        args = [arg for arg in fr.block() if arg.is_argument]

        args_s = [str(arg.value(fr)) for arg in args]
        oper = str(args[1].value(fr))

        if   oper == "__metal_oper_enter_case"   : self.enter_case   (args, fr)
        elif oper == "__metal_oper_exit_case"    : self.exit_case    (args, fr)
        elif oper == "__metal_oper_enter_ranged" : self.enter_ranged (args, fr)
        elif oper == "__metal_oper_exit_ranged"  : self.exit_ranged  (args, fr)
        elif oper == "__metal_oper_log"          : self.log          (args, fr)
        elif oper == "__metal_oper_checkpoint"   : self.checkpoint   (args, fr)
        elif oper == "__metal_oper_message"      : self.message      (args, fr)
        elif oper == "__metal_oper_plain"        : self.plain        (args, fr)
        elif oper == "__metal_oper_predicate"    : self.predicate    (args, fr)
        elif oper == "__metal_oper_equal"        : self.equal        (args, fr)
        elif oper == "__metal_oper_not_equal"    : self.not_equal    (args, fr)
        elif oper == "__metal_oper_close"        : self.close        (args, fr)
        elif oper == "__metal_oper_close_rel"    : self.close_rel    (args, fr)
        elif oper == "__metal_oper_close_perc"   : self.close_perc   (args, fr)
        elif oper == "__metal_oper_ge"           : self.ge           (args, fr)
        elif oper == "__metal_oper_greater"      : self.greater      (args, fr)
        elif oper == "__metal_oper_le"           : self.le           (args, fr)
        elif oper == "__metal_oper_lesser"       : self.lesser       (args, fr)
        elif oper == "__metal_oper_exception"    : self.exception    (args, fr)
        elif oper == "__metal_oper_any_exception": self.any_exception(args, fr)
        elif oper == "__metal_oper_no_exception" : self.no_exception (args, fr)
        elif oper == "__metal_oper_no_exec"      : self.no_exec      (args, fr)
        elif oper == "__metal_oper_exec"         : self.exec         (args, fr)
        elif oper == "__metal_oper_report"       : self.report       (args, fr)

    def exec(self, args, frame):
        selectTestFormat.sink.exec(args, frame)

metal_test_backend()

