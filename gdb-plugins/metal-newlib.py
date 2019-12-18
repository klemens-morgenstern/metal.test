import traceback

import gdb
import os



class open_flags:

    def __init__(self):
        try: self.o_append   = int(str(gdb.parse_and_eva("O_APPEND")  ))
        except: pass
        try: self.o_creat    = int(str(gdb.parse_and_eva("O_CREAT")   ))
        except: pass
        try: self.o_excl     = int(str(gdb.parse_and_eva("O_EXCL")    ))
        except: pass
        try: self.o_noctty   = int(str(gdb.parse_and_eva("O_NOCTTY")  ))
        except: pass
        try: self.o_nonblock = int(str(gdb.parse_and_eva("O_NONBLOCK")))
        except: pass
        try: self.o_sync     = int(str(gdb.parse_and_eva("O_SYNC")    ))
        except: pass
        try: self.o_async    = int(str(gdb.parse_and_eva("O_ASYNC")   ))
        except: pass
        try: self.o_cloexec  = int(str(gdb.parse_and_eva("O_CLOEXEC") ))
        except: pass
        try: self.o_direct   = int(str(gdb.parse_and_eva("O_DIRECT")  ))
        except: pass
        try: self.o_directory= int(str(gdb.parse_and_eva("O_DIRECTORY")))
        except: pass
        try: self.o_dsync    = int(str(gdb.parse_and_eva("O_DSYNC")   ))
        except: pass
        try: self.o_largefile= int(str(gdb.parse_and_eva("O_LARGEFILE")))
        except: pass
        try: self.o_noatime  = int(str(gdb.parse_and_eva("O_NOATIME") ))
        except: pass
        try: self.o_ndelay   = int(str(gdb.parse_and_eva("O_NDELAY")  ))
        except: pass
        try: self.o_path     = int(str(gdb.parse_and_eva("O_PATH")    ))
        except: pass
        try: self.o_trunc    = int(str(gdb.parse_and_eva("O_TRUNC")   ))
        except: pass
        try: self.o_rdonly   = int(str(gdb.parse_and_eva("O_RDONLY")  ))
        except: pass
        try: self.o_wronly   = int(str(gdb.parse_and_eva("O_WRONLY")  ))
        except: pass
        try: self.o_rdwr     = int(str(gdb.parse_and_eva("O_RDWR")    ))
        except: pass
        try: self.s_iread    = int(str(gdb.parse_and_eva("S_IREAD")   ))
        except: pass
        try: self.s_iwrite   = int(str(gdb.parse_and_eva("S_IWRITE")  ))
        except: pass
        try: self.s_iwusr    = int(str(gdb.parse_and_eva("S_IWUSR")   ))
        except: pass
        try: self.s_ixusr    = int(str(gdb.parse_and_eva("S_IXUSR")   ))
        except: pass
        try: self.s_irwxg    = int(str(gdb.parse_and_eva("S_IRWXG")   ))
        except: pass
        try: self.s_irgrp    = int(str(gdb.parse_and_eva("S_IRGRP")   ))
        except: pass
        try: self.s_iwgrp    = int(str(gdb.parse_and_eva("S_IWGRP")   ))
        except: pass
        try: self.s_ixgrp    = int(str(gdb.parse_and_eva("S_IXGRP")   ))
        except: pass
        try: self.s_irwxo    = int(str(gdb.parse_and_eva("S_IRWXO")   ))
        except: pass
        try: self.s_iroth    = int(str(gdb.parse_and_eva("S_IROTH")   ))
        except: pass
        try: self.s_iwoth    = int(str(gdb.parse_and_eva("S_IWOTH")   ))
        except: pass
        try: self.s_ixoth    = int(str(gdb.parse_and_eva("S_IXOTH")   ))
        except: pass
        try: self.s_isuid    = int(str(gdb.parse_and_eva("S_ISUID")   ))
        except: pass
        try: self.s_isgid    = int(str(gdb.parse_and_eva("S_ISGID")   ))
        except: pass
        try: self.s_isvtx    = int(str(gdb.parse_and_eva("S_ISVTX")   ))
        except: pass

    def get_flags(self, in_):

        out = 0
        out |= os.O_BINARY
        if in_ & self.o_append   : out |= os.O_APPEND
        if in_ & self.o_creat    : out |= os.O_CREAT
        if in_ & self.o_excl     : out |= os.O_EXCL
        if in_ & self.o_rdonly   : out |= os.O_RDONLY
        if in_ & self.o_wronly   : out |= os.O_WRONLY
        if in_ & self.o_rdwr     : out |= os.O_RDWR
        if in_ & self.o_noctty   : out |= os.O_NOCTTY
        if in_ & self.o_nonblock : out |= os.O_NONBLOCK
        if in_ & self.o_sync     : out |= os.O_SYNC
        if in_ & self.o_async    : out |= os.O_ASYNC
        if in_ & self.o_cloexec  : out |= os.O_CLOEXEC
        if in_ & self.o_direct   : out |= os.O_DIRECT
        if in_ & self.o_directory: out |= os.O_DIRECTORY
        if in_ & self.o_dsync    : out |= os.O_DSYNC
        if in_ & self.o_largefile: out |= os.O_LARGEFILE
        if in_ & self.o_noatime  : out |= os.O_NOATIME
        if in_ & self.o_ndelay   : out |= os.O_NDELAY
        if in_ & self.o_path     : out |= os.O_PATH
        if in_ & self.o_trunc    : out |= os.O_TRUNC

        return out


    def get_mode(self, in_):

        out = 0
        if in_ & self.s_irwxu:   out |= os.S_IREAD | os.S_IWRITE
        if in_ & self.s_irusr:   out |= os.S_IREAD
        if in_ & self.s_iwusr:   out |= os.S_IWRITE
        if in_ & self.s_iread:   out |= os.S_IREAD
        if in_ & self.s_iwrite:  out |= os.S_IWRITE
        if in_ & self.s_irwxu:   out |= os.S_IRWXU
        if in_ & self.s_irusr:   out |= os.S_IRUSR
        if in_ & self.s_iwusr:   out |= os.S_IWUSR
        if in_ & self.s_ixusr:   out |= os.S_IXUSR
        if in_ & self.s_irwxg:   out |= os.S_IRWXG
        if in_ & self.s_irgrp:   out |= os.S_IRGRP
        if in_ & self.s_iwgrp:   out |= os.S_IWGRP
        if in_ & self.s_ixgrp:   out |= os.S_IXGRP
        if in_ & self.s_irwxo:   out |= os.S_IRWXO
        if in_ & self.s_iroth:   out |= os.S_IROTH
        if in_ & self.s_iwoth:   out |= os.S_IWOTH
        if in_ & self.s_ixoth:   out |= os.S_IXOTH
        if in_ & self.s_isuid:   out |= os.S_ISUID
        if in_ & self.s_isgid:   out |= os.S_ISGID
        if in_ & self.s_isvtx:   out |= os.S_ISVTX
        return out

class seek_flags:

    def __init__(self):
        try: self.seek_set = int(str(gdb.parse_and_eval("SEEK_SET")));
        except: pass
        try: self.seek_cur = int(str(gdb.parse_and_eval("SEEK_CUR")));
        except: pass
        try: self.seek_end = int(str(gdb.parse_and_eval("SEEK_END")));
        except: pass

    def get_flags(self, in_):
        out = 0;
        if in_ & self.seek_set: out |= os.SEEK_SET;
        if in_ & self.seek_cur: out |= os.SEEK_CUR;
        if in_ & self.seek_end: out |= os.SEEK_END;

        return out;

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

            type = str(args[0].value())
            oper = str(args[1].value(fr))[len("metal_func_"):]

            try:
                getattr(self, oper)(args, fr)
            except OSError as e:
                gdb.execute("set var errno = {}".format(e.errno))
            except Exception as e:
                gdb.write("Internal error {}\n".format(e), gdb.STDERR)

            gdb.post_event(lambda: gdb.execute("continue"))
        except gdb.error as e:
            gdb.write("Error in metal-newlib.py: {}".format(e))
            traceback.print_exc()
            raise e

    def close(self, args, frame):
        fd = int(str(args[3].value()))
        res = os.close(fd)
        gdb.execute("return {}".format(res))

    def fstat(self, args, frame):
        fd = int(str(args[3].value()))
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
        fd = int(str(args[3].value()))
        gdb.execute("return {}".format(os.isatty(fd)))

    def link(self, args, frame):
        fd = int(str(args[3].value()))
        existing = str(args[1].value())
        _new = str(args[2].value())
        ret = os.link(existing, _new)
        gdb.execute("return {}".format(ret))

    def lseek(self, args, frame):
        fd = int(str(args[3].value()))
        ptr = int(str(args[4].value()))
        dir_in = int(str(args[5].value()))

        if self.seek_flags is None:
            self.seek_flags = seek_flags()

        dir = self.seek_flags.get_flags(dir_in)
        ret = os.lseek(fd, ptr, dir)
        gdb.execute("return {}".format(ret))

    def open(self, args, frame):
        file = str(args[1].value())
        flags_in = int(str(args[3].value()))
        mode_in = int(str(args[3].value()))

        if self.open_flags is None:
            self.open_flags = open_flags

        flags = self.open_flags.get_flags(flags_in)
        mode  = self.open_flags.get_mode (mode_in)
        fd = os.open(file, flags, mode)
        gdb.execute("return {}".format(fd))

    def read(self, args, frame):
        fd = int(str(args[3].value()))
        len = int(str(args[4].value()))
        ptr = int(str(args[6].value()))

        buf = os.read(fd, len)

        gdb.selected_inferior().write_memory(ptr, buf, len(buf))
        gdb.execute("return {}".format(len(buf)))

    def stats(self, args, frame):
        file = str(args[1].value())
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
        existing = str(args[1].value())
        _new = str(args[2].value())

        ret = os.symlink(existing, _new)
        gdb.execute("return {}".format(ret))

    def unlink(self, args, frame):
        name = str(args[6].value())
        ret = os.unlink(name)
        gdb.execute("return {}".format(ret))

    def write(self, args, frame):
        fd = int(str(args[3].value()))
        len = int(str(args[4].value()))
        ptr = int(str(args[6].value()))

        buf = gdb.selected_inferior().read_memory(ptr, len)
        ret = os.write(fd, buf)
        gdb.execute("return {}".format(ret))
