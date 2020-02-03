import traceback

import gdb
import os
import stat

class WorkingDir:
    def __init__(self):
        self.value = os.getenv("METAL_CI_SOURCE_DIR")

    def map(self, path):
        if self.value is None or not path.startswith(self.value):
            return path
        else:
            return path.replace(self.value, os.getcwd())

workingDir = WorkingDir()

class open_flags:

    def __init__(self):
        self.o_append    = 0x0008
        self.o_creat     = 0x0200
        self.o_excl      = 0x0800
        self.o_noctty    = 0x8000
        self.o_nonblock  = 0x4000
        self.o_sync      = 0x2000
        self.o_async     = 0x0040
        self.o_cloexec   = 0x40000
        self.o_direct    = 0x80000
        self.o_directory = 0x200000
        self.o_dsync     = 0
        self.o_largefile = 0
        self.o_noatime   = 0
        self.o_ndelay    = 0
        self.o_path      = 0
        self.o_trunc     = 0x0400
        self.o_rdonly    = 0
        self.o_wronly    = 1
        self.o_rdwr      = 2
        self.s_iread     = 0o000400
        self.s_iwrite    = 0o000200
        self.s_iwusr     = 0o000200
        self.s_ixusr     = 0o000100
        self.s_irgrp     = 0o000040
        self.s_iwgrp     = 0o000020
        self.s_ixgrp     = 0o000010
        self.s_irwxg     = self.s_irgrp | self.s_iwgrp | self.s_ixgrp
        self.s_iroth     = 0o000004
        self.s_iwoth     = 0o000002
        self.s_ixoth     = 0o000001
        self.s_irwxo     = self.s_iroth | self.s_iwoth | self.s_ixoth
        self.s_isuid     = 0o004000
        self.s_isgid     = 0o002000
        self.s_isvtx     = 0o001000
        self.s_irusr     = 0o000400
        self.s_irwxu     = self.s_irusr | self.s_iwusr | self.s_ixusr


    def get_flags(self, in_):

        out = 0
        try:
            out |= os.O_BINARY
        except:
            pass
        if in_ & self.o_append   : out |= os.O_APPEND
        if in_ & self.o_creat    : out |= os.O_CREAT
        if in_ & self.o_excl     : out |= os.O_EXCL
        if in_ & self.o_rdonly   : out |= os.O_RDONLY
        if in_ & self.o_wronly   : out |= os.O_WRONLY
        if in_ & self.o_rdwr     : out |= os.O_RDWR
        try:
            if in_ & self.o_noctty   : out |= os.O_NOCTTY
        except: pass
        try:
            if in_ & self.o_nonblock : out |= os.O_NONBLOCK
        except: pass
        try:
            if in_ & self.o_sync     : out |= os.O_SYNC
        except: pass
        try:
            if in_ & self.o_async    : out |= os.O_ASYNC
        except: pass
        try:
            if in_ & self.o_cloexec  : out |= os.O_CLOEXEC
        except: pass
        try:
            if in_ & self.o_direct   : out |= os.O_DIRECT
        except: pass
        try:
            if in_ & self.o_directory: out |= os.O_DIRECTORY
        except: pass
        try:
            if in_ & self.o_dsync    : out |= os.O_DSYNC
        except: pass
        try:
            if in_ & self.o_largefile: out |= os.O_LARGEFILE
        except: pass
        try:
            if in_ & self.o_noatime  : out |= os.O_NOATIME
        except: pass
        try:
            if in_ & self.o_ndelay   : out |= os.O_NDELAY
        except: pass
        try:
            if in_ & self.o_path     : out |= os.O_PATH
        except: pass
        if in_ & self.o_trunc    : out |= os.O_TRUNC
        return out


    def get_mode(self, in_):

        out = 0
        if in_ & self.s_irwxu:   out |= stat.S_IREAD | stat.S_IWRITE
        if in_ & self.s_iwusr:   out |= stat.S_IWRITE
        if in_ & self.s_iread:   out |= stat.S_IREAD
        if in_ & self.s_iwrite:  out |= stat.S_IWRITE
        if in_ & self.s_irwxu:   out |= stat.S_IRWXU
        if in_ & self.s_irusr:   out |= stat.S_IRUSR
        if in_ & self.s_iwusr:   out |= stat.S_IWUSR
        if in_ & self.s_ixusr:   out |= stat.S_IXUSR
        if in_ & self.s_irwxg:   out |= stat.S_IRWXG
        if in_ & self.s_irgrp:   out |= stat.S_IRGRP
        if in_ & self.s_iwgrp:   out |= stat.S_IWGRP
        if in_ & self.s_ixgrp:   out |= stat.S_IXGRP
        if in_ & self.s_irwxo:   out |= stat.S_IRWXO
        if in_ & self.s_iroth:   out |= stat.S_IROTH
        if in_ & self.s_iwoth:   out |= stat.S_IWOTH
        if in_ & self.s_ixoth:   out |= stat.S_IXOTH
        if in_ & self.s_isuid:   out |= stat.S_ISUID
        if in_ & self.s_isgid:   out |= stat.S_ISGID
        if in_ & self.s_isvtx:   out |= stat.S_ISVTX
        return out

class seek_flags:

    def __init__(self):
        self.seek_set = 0
        self.seek_cur = 1
        self.seek_end = 2

    def get_flags(self, in_):
        out = 0
        if in_ & self.seek_set: out |= os.SEEK_SET
        if in_ & self.seek_cur: out |= os.SEEK_CUR
        if in_ & self.seek_end: out |= os.SEEK_END

        return out

class metal_test_backend(gdb.Breakpoint):
    def __init__(self):
        gdb.Breakpoint.__init__(self, "metal_func_stub")
        self.fd_map = {}
        self.seek_flags = None
        self.open_flags = None

    def stop(self):
        try:
            fr = gdb.selected_frame()
            args = [arg for arg in fr.block() if arg.is_argument]
            oper = str(args[0].value(fr))[len("metal_func_"):]

            try:
                getattr(self, oper)(args, fr)
            except OSError as e:
                gdb.write("**NewLib** Error executing {}: {}\n".format(oper, e))
                try: gdb.execute("set var (*_errno()) = {}".format(e.errno))
                except: pass
            except Exception as e:
                gdb.write("Internal error [{}] {}\n".format(oper, e), gdb.STDERR)

            return False
        except gdb.error as e:
            gdb.write("Error in metal-newlib.py: {}".format(e))
            traceback.print_exc()
            raise e

    def close(self, args, frame):
        fd = int(str(args[3].value(frame)))
        try:
            os.close(fd)
            gdb.execute("return 0")
        except:
            gdb.execute("return -1")
            raise

    def fstat(self, args, frame):
        fd = int(str(args[3].value(frame)))
        try:
            st = os.fstat(fd)
            gdb.execute("set_var")

            gdb.execute("set var arg7->st_dev   = {}", st.st_dev)
            gdb.execute("set var arg7->st_ino   = {}", st.st_ino)
            gdb.execute("set var arg7->st_mode  = {}", st.st_mode)
            gdb.execute("set var arg7->st_nlink = {}", st.st_nlink)
            gdb.execute("set var arg7->st_uid   = {}", st.st_uid)
            gdb.execute("set var arg7->st_gid   = {}", st.st_gid)
            gdb.execute("set var arg7->st_rdev  = {}", st.st_rdev)
            gdb.execute("set var arg7->st_size  = {}", st.st_size)
            gdb.execute("set var arg7->st_atime = {}", st.st_atime)
            gdb.execute("set var arg7->st_mtime = {}", st.st_mtime)
            gdb.execute("set var arg7->st_ctime = {}", st.st_ctime)

            gdb.execute("return 0")
        except OSError as e:
            gdb.execute("return {}".format(e.errno))

    def isattay(self, args, frame):
        fd = int(str(args[3].value(frame)))
        gdb.execute("return {}".format(os.isatty(fd)))

    def link(self, args, frame):
        fd = int(str(args[3].value(frame)))
        existing = workingDir.map(str(args[1].value(frame)))
        _new = workingDir.map(str(args[2].value(frame)))
        ret = os.link(existing, _new)
        gdb.execute("return {}".format(ret))

    def lseek(self, args, frame):
        fd = int(str(args[3].value(frame)))
        ptr = int(str(args[4].value(frame)).split(' ')[0], 0)
        dir_in = int(str(args[5].value(frame)))

        if self.seek_flags is None:
            self.seek_flags = seek_flags()

        dir = self.seek_flags.get_flags(dir_in)
        ret = os.lseek(fd, ptr, dir)
        gdb.execute("return {}".format(ret))

    def open(self, args, frame):
        file = workingDir.map(str(args[1].value(frame).string()))
        flags_in = int(str(args[3].value(frame)))
        mode_in = int(str(args[3].value(frame)))

        if self.open_flags is None:
            self.open_flags = open_flags()

        flags = self.open_flags.get_flags(flags_in)
        mode = self.open_flags.get_mode(mode_in)

        if flags & os.O_CREAT:
            mode |= stat.S_IRUSR | stat.S_IRGRP | stat.S_IRWXO
            if file.endswith(".gcda"):
                base = os.path.dirname(file)
                if not os.path.exists(base):
                    gdb.write("**newlib**: Creating folder {}\n".format(base))
                    os.makedirs(base)


        fd = os.open(file, flags, mode)
        gdb.execute("return {}".format(fd))

    def read(self, args, frame):
        fd = int(str(args[3].value(frame)))
        len_ = int(str(args[4].value(frame)))
        ptr = int(str(args[6].value(frame)).split(' ')[0], 0)

        buf = os.read(fd, len_)

        gdb.selected_inferior().write_memory(ptr, buf, len(buf))
        gdb.execute("return {}".format(len(buf)))

    def stat(self, args, frame):
        file = workingDir.map(str(args[1].value(frame)))
        st = os.stat(file)

        gdb.execute("set var arg7->st_dev   ={}".format(st.st_dev))
        gdb.execute("set var arg7->st_ino   ={}".format(st.st_ino))
        gdb.execute("set var arg7->st_mode  ={}".format(st.st_mode))
        gdb.execute("set var arg7->st_nlink ={}".format(st.st_nlink))
        gdb.execute("set var arg7->st_uid   ={}".format(st.st_uid))
        gdb.execute("set var arg7->st_gid   ={}".format(st.st_gid))
        gdb.execute("set var arg7->st_rdev  ={}".format(st.st_rdev))
        gdb.execute("set var arg7->st_size  ={}".format(st.st_size))
        gdb.execute("set var arg7->st_atime ={}".format(st.st_atime))
        gdb.execute("set var arg7->st_mtime ={}".format(st.st_mtime))
        gdb.execute("set var arg7->st_ctime ={}".format(st.st_ctime))

        gdb.execute("return 0")

    def symblink(self, args, frame):
        existing = workingDir.map(str(args[1].value(frame)))
        _new = workingDir.map(str(args[2].value(frame)))

        ret = os.symlink(existing, _new)
        gdb.execute("return {}".format(ret))

    def unlink(self, args, frame):
        name = workingDir.map(str(args[6].value(frame)))
        ret = os.unlink(name)
        gdb.execute("return {}".format(ret))

    def write(self, args, frame):
        fd = int(str(args[3].value(frame)))
        len_ = int(str(args[4].value(frame)))
        ptr = int(str(args[6].value(frame)).split(' ')[0], 0)
        buf = gdb.selected_inferior().read_memory(ptr, len_)
        ret = os.write(fd, buf)
        gdb.execute("return {}".format(ret))



mtb = metal_test_backend()