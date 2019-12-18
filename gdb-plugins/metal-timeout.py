import sys
import gdb
import threading
import datetime
import time
import signal
import os

class Timeout(gdb.Parameter):
    def __init__(self):
        super(Timeout, self).__init__("metal-timeout",
                                              gdb.COMMAND_DATA,
                                              gdb.PARAM_UINTEGER)
        self.value = 0
        self.thread = None
        self.timeout = None
        self.gdb_running = False
        self.i_am_interrupting = False
        self.gdb_exited = False

    set_doc = '''Set a timeout for the gdb execution.'''
    show_doc = '''This parameter sets a time out for the gdb execution.'''

    def get_set_string(self):
        if self.value and self.value > 0:
            self.timeout = datetime.datetime.now() + datetime.timedelta(seconds=self.value)
        else:
            self.timeout = None
        if self.value and not self.thread:
            self.thread = threading.Thread(target=self.work, args=(None,))
            self.thread.start()
        return "Timeout set to {}".format(self.timeout)

    def work(self, dummy):
        while not self.gdb_exited and (self.timeout is None or self.timeout > datetime.datetime.now()):
            time.sleep(1)

        if self.gdb_exited or self.timeout is None:
            return
        if self.gdb_running:
            self.i_am_interrupting = True
            gdb.post_event(self.interrupt)
        else:
            gdb.post_event(self.quit)


    def interrupt(self):
        raise KeyboardInterrupt()

    def cont(self, ev): self.gdb_running = True
    def stop(self, ev):
        self.gdb_running = False
        if self.i_am_interrupting:
            gdb.post_event(self.quit)


    def quit(self):
        gdb.write("Timeout reached, interrupting execution\n", gdb.STDERR)
        gdb.execute("quit 2")


    def exit(self, ev): self.gdb_exited = True


timeout = Timeout()

gdb.events.cont.connect(timeout.cont)
gdb.events.stop.connect(timeout.stop)
gdb.events.exited.connect(timeout.exit)
