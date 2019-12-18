import json
import sys
import traceback

import gdb
import ctypes


class ManualDisable(gdb.Parameter):
    def __init__(self):
        super(ManualDisable, self).__init__("metal-calltrace-manual-disable",
                                            gdb.COMMAND_DATA,
                                            gdb.PARAM_BOOLEAN)
        self.value = True

    set_doc = '''Manually disabling for the timestamp.'''
    show_doc = '''Manually disabling for the timestamp.'''


manual_dis = ManualDisable()


class SelectJsonSink(gdb.Parameter):
    def __init__(self):
        super(SelectJsonSink, self).__init__("metal-calltrace-json-file",
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
    show_doc = '''This sets the calltrace data output sink.'''


trace_json = SelectJsonSink()


class TraceAll(gdb.Parameter):
    def __init__(self):
        super(TraceAll, self).__init__("metal-calltrace-all",
                                       gdb.COMMAND_DATA,
                                       gdb.PARAM_BOOLEAN)
        self.value = True

    set_doc = '''Trace all calls.'''
    show_doc = '''Trace all calls.'''


log_all = TraceAll()


class Profile(gdb.Parameter):
    def __init__(self):
        super(Profile, self).__init__("metal-calltrace-timestamp",
                                       gdb.COMMAND_DATA,
                                       gdb.PARAM_BOOLEAN)
        self.value = True

    set_doc = '''Enable profiling.'''
    show_doc = '''Enable profiling.'''


profile = Profile()


class Minimal(gdb.Parameter):
    def __init__(self):
        super(Minimal, self).__init__("metal-calltrace-minimal",
                                       gdb.COMMAND_DATA,
                                       gdb.PARAM_BOOLEAN)
        self.value = True

    set_doc = '''Only output the result of the actual calltraces.'''
    show_doc = '''Only output the result of the actual calltraces.'''


minimal = Minimal()


class Depth(gdb.Parameter):
    def __init__(self):
        super(Depth, self).__init__("metal-calltrace-depth",
                                       gdb.COMMAND_DATA,
                                       gdb.PARAM_INTEGER)
        self.value = 3

    set_doc = '''Maximum depth of the calltrace recording.'''
    show_doc = '''Maximum depth of the calltrace recording.'''


depth = Depth()


class calltrace_clone_entry:
    def __init__(self, address, block):
        self.address = address
        self.block = block


class calltrace_clone:
    def __init__(self, location, fn, content, repeat, skip):
        self.location = location
        self.fn = fn
        self.content = content
        self.repeat = repeat
        self.skip = skip

        self.to_skip = self._skip
        self.errors = 0
        self.current_position = 0
        self.start_depth = -1
        self.repeated = 0
        self.ovl = False

    inactive = property(lambda self: self.start_depth == -1)

    def add_skipped(self):
        self.to_skip -= 1

    def start(self, depth):
        self.start_depth = depth
        self.ovl = False

    def my_depth(self, value):
        return self.start_depth == value

    def my_child(self, value):
        return (self.start_depth + 1) == value

    def previous(self):
        if len(self.content) >= self.current_position > 0:
            return self.content[self.current_position]
        return None

    def check(self, this_fn):
        if self.current_position >= len(self.content):
            self.ovl = True
            self.errors += 1
            return False

        ptr = self.content[self.current_position].address
        self.current_position += 1
        if ptr != this_fn and ptr != 0:
            self.errors += 1
            return False
        else:
            return True

    def stop(self):
        self.start_depth = -1
        res = True
        if self.current_position != len(self.content):
            self.errors += 1
            res = False
        self.current_position = 0
        self.repeated += 1
        return res


class metal_calltrace(gdb.Breakpoint):

    def __fn(self, ptr, func):
        if func.function:
            return "@0x{:X}:{}".format(ptr, str(func.function.value()))
        else:
            return "@0x{:X}:***unknown function***".format(ptr)

    def __loc(self, ptr, sal):
        if sal and sal.symtab:
            return "{}({})".format(sal.line, sal.symtab.filename).replace("\\\\", "\\")
        else:
            return "***unknown location***(0)"

    def __address_info(self, ptr, loc, block):
        return {
            "file": loc.symtab.filename,
            "function": block.function,
            "line": loc.line,
            "ptr": ptr,
            "pc": loc.pc
        }

    def __init__(self):
        gdb.Breakpoint.__init__(self, "__metal_profile")
        self.timestamp_available = True
        self.errors = []
        self.cts = []

    def stop(self):
        try:
            frame = gdb.selected_frame()
            args = [arg for arg in frame.block() if arg.is_argument]

            for arg in args:
                exit_code = arg.value(frame)
                break

            gdb.post_event(lambda: self.exit(exit_code))
        except gdb.error as e:
            gdb.write("Error in metal-exitcode.py: {}".format(e))
            traceback.print_exc()
            raise e

    def enter(self, args, frame):

        function_arg = args[1]
        callsite_arg = args[2]
        function_ptr = int(function_arg.value(), 16)
        callsite_ptr = int(callsite_arg.value(), 16)

        function_loc = gdb.find_pc_line(function_ptr)
        callsite_loc = gdb.find_pc_line(callsite_ptr)

        function_block = gdb.block_for_pc(function_ptr)
        callsite_block = gdb.block_for_pc(callsite_ptr)

        ts = self.timestamp(args, frame)

        if not minimal.value:
            hrf = "metal.calltrace entering function [{}]".format(self.__fn(function_ptr, function_block))
            if ts:
                hrf += ", with timestamp {}".format(ts)
            if callsite_block:
                hrf += ", at " + self.__loc(callsite_ptr, callsite_loc)
            hrf += "\n"
            gdb.write(hrf)
            msg = {
                "mode": "enter",
                "this_fn_ptr": "0x" + "".join([str(hex(x)) for x in function_ptr.to_bytes()]),
                "call_site_ptr": "0x" + "".join([str(hex(x)) for x in callsite_ptr.to_bytes()]),
                "this_fn": self.__address_info(function_ptr, function_loc, function_block),
                "call_site": self.__address_info(callsite_ptr, callsite_loc, callsite_block)
            }
            if ts:
                msg["timestamp"] = ts

            self.errors.append(msg)

        if len(self.cts) == 0:
            return

        depth = str(gdb.parse_and_eval("__metal_calltrace_depth"))

        for ct in self.cts:
            if ct.inactive and ct.fn == function_ptr and (ct.repeat > ct.repeated or ct.repeat == 0):
                if ct.to_skip:
                    ct.add_skipped()
                else:
                    ct.start(depth)
            elif ct.my_child(depth):
                if not ct.check(function_ptr):
                    if ct.ovl:
                        gdb.write("metal.calltrace.error overflow in calltrace @0x{:X} with size {} at pos [{}]\n"
                                  .format(function_ptr, len(ct.content),
                                          self.__fn(function_ptr, function_block)))

                        self.errors.append({
                            "mode": "error",
                            "type": "overflow",
                            "calltrace_loc": ct.location,
                            "function_ptr": function_ptr,
                            "function": self.__address_info(function_ptr, function_loc, function_block),
                            "calltrace": {
                                "location": ct.location,
                                "repeated": ct.repeated
                            }})
                    else:
                        gdb.write("metal.calltrace.error mismatch in calltrace @0x{:X} at pos {} {{[{}] != [{}]}}\n"
                                  .format(function_ptr, ct.current_position, self.__fn(function_ptr, function_block),
                                          self.__fn(ct.previous().address, ct.previous().block)))

                        self.errors.append({
                            "mode": "error",
                            "type": "mismatch",
                            "calltrace_loc": ct.location,
                            "function_ptr": function_ptr,
                            "function": self.__address_info(function_ptr, function_loc, function_block),
                            "calltrace": {
                                "location": ct.location,
                                "repeated": ct.repeated
                            }})

    def exit(self, args, frame):
        function_arg = args[1]
        callsite_arg = args[2]
        function_ptr = int(function_arg.value(), 16)
        callsite_ptr = int(callsite_arg.value(), 16)

        function_loc = gdb.find_pc_line(function_ptr)
        callsite_loc = gdb.find_pc_line(callsite_ptr)

        function_block = gdb.block_for_pc(function_ptr)
        callsite_block = gdb.block_for_pc(callsite_ptr)

        ts = self.timestamp(args, frame)

        if not minimal.value:
            hrf = "metal.calltrace exiting function [{}]".format(self.__fn(function_ptr, function_block))
            if ts:
                hrf += ", with timestamp {}".format(ts)
            if callsite_block:
                hrf += ", at " + self.__loc(callsite_ptr, callsite_loc)
            hrf += "\n"
            gdb.write(hrf)
            msg = {
                "mode": "exit",
                "this_fn_ptr": "0x" + "".join([str(hex(x)) for x in function_ptr.to_bytes()]),
                "call_site_ptr": "0x" + "".join([str(hex(x)) for x in callsite_ptr.to_bytes()]),
                "this_fn": self.__address_info(function_ptr, function_loc, function_block),
                "call_site": self.__address_info(callsite_ptr, callsite_loc, callsite_block)
            }
            if ts:
                msg["timestamp"] = ts

        if len(self.cts):
            return

        depth = str(gdb.parse_and_eval("__metal_calltrace_depth"))

        for ct in self.cts:
            if ct.my_depth(depth):
                pos = ct.current_position
                if ct.stop():
                    gdb.write("metal.calltrace.error incomplete in calltrace @0x{:X} stopped {}/{}\n"
                              .format(function_ptr, pos, len(ct.content)))
                    self.errors.append({
                        "mode": "error",
                        "type": "incomplete",
                        "position": pos,
                        "size": len(ct.content),
                        "calltrace": {
                            "location": ct.location,
                            "repeated": ct.repeated
                        }})

    def set(self, args, frame):
        ct_ptr = str(args[1].value())
        ct_str = "((struct metal_calltrace_*){})".format(ct_ptr)
        addr = int(str(gdb.parse_and_eval("{}->fn".format(ct_str))), 16)
        fn = calltrace_clone_entry(addr, gdb.block_for_pc(addr))

        sz = int(str(gdb.parse_and_eval("{}->content_size".format(ct_str))))
        content = []
        for i in range(0, sz):
            p = int(str(gdb.parse_and_eval("{}->content[{}]".format(ct_str, i))), 16)
            if p != 0:
                content.append(calltrace_clone_entry(p, gdb.block_for_pc(p)))
            else:
                content.append(calltrace_clone_entry(p, None))

        repeat = int(str(gdb.parse_and_eval("{}->repeat".format(ct_str))))
        skip   = int(str(gdb.parse_and_eval("{}->skip".  format(ct_str))))
        ts = self.timestamp(args, frame)

        cc = calltrace_clone(ct_ptr, fn, content, repeat, skip)
        self.cts.append(cc)

        ct_to_str = lambda val : "**any**" if val.addess == 0 else "[{}]".format(self.__fn(val.address, val.block))
        hrf = "metal.calltrace   registered calltrace @0x{:X} [{}]: {".format(cc.location, self.__fn(cc.fn.address, cc.fn.block))
        hrf += ", ".join([ct_to_str(c) for c in cc.content])
        hrf += "}"
        if ts:
            hrf += ", with timestamp {}".format(ts)
        hrf += "\n"
        gdb.write(hrf)

        js_c = lambda val: {
            "address" : val.address,
            "function" : str(val.block.function),
            "file": val.block.function.symtab.filename,
            "line" : gdb.find_pc_line(val.address).line}
        js = {
            "mode":"set",
            "calltrace": {
                "location": cc.location,
                "repeat": cc.repeat,
                "skip": cc.skip,
                "fn" : {
                    "address" : cc.fn.address,
                    "function" : str(cc.fn.block.function),
                    "file": cc.fn.block.function.symtab.filename,
                    "line" : gdb.find_pc_line(cc.fn.address).line
                },
                "content": [js_c(c) for c in  cc.content]
            }
        }

        if ts:
            js["timestamp"] = ts

        self.errors.append(js)


    def reset(self, args, frame):
        ct_ptr = int(args[1].value(), 16)
        ts = self.timestamp(args, frame)

        for rem in [ct for ct in self.cts if ct.location == ct_ptr]:
            hrf =  "metal.calltrace unregistered calltrace @0x{:X} executed {} times, with {} errors".format(rem.location, rem.repeated, rem.errors)
            if ts:
                hrf += ", with timestamp {}".format(ts)
            hrf += "\n"
            gdb.write(hrf)

            res = {
                "mode": "reset",
                "calltrace": {
                    "location": rem.location,
                    "repeated": rem.repeated}}
            if ts:
                res["timestamp"] = ts
            self.errors.append(res)
            self.cts.remove(rem)

    def timestamp(self, args, frame):
        if not profile.value or not self.timestamp_available:
            return None

        if manual_dis.value:
            self.enabled = False
            ct_size = str(gdb.parse_and_eval("__metal_calltrace_size"))
            gdb.execute("set val __metal_calltrace_size = 0")

        res = None

        try:
            res = str(gdb.parse_and_eval("metal_timestamp()"))
        except:
            self.timestamp_available = False
            self.errors.append({"mode": "error", "type": "missing-timestamp"})
            gdb.write("metal.calltrace.error timestamp not available\n")

        if manual_dis.value:
            gdb.execute("set val __metal_calltrace_size = ct_size")
            self.enabled = True

        if res:
            return int(res)
        else:
            return None


_call_trace = metal_calltrace()


def exit_event(event):
    if trace_json.value is not None:
        if trace_json.sink is not None:
            trace_json.sink.write(json.dumps(_call_trace.errors))
            return
        with open(trace_json.value, "w") as f:
            f.write(json.dumps(_call_trace.errors))


gdb.events.exited.connect(exit_event)
