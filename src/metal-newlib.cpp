#include <boost/dll/alias.hpp>
#include <boost/system/api_config.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <metal/debug/break_point.hpp>
#include <metal/debug/frame.hpp>
#include <vector>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <iostream>
#include <metal/debug/plugin.hpp>

#if defined(BOOST_WINDOWS_API)
#include <windows.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <unistd.h>
#endif

#if !defined(O_DIRECT)
#define O_DIRECT 0
#endif
#if !defined(O_LARGEFILE)
#define O_LARGEFILE 0
#endif
#if !defined(O_NOATIME)
#define O_NOATIME 0
#endif
#if !defined(O_PATH)
#define O_PATH 0
#endif

using namespace metal::debug;

#if defined(BOOST_POSIX_API)
#define call(Func, Args...) :: Func ( Args )
#define flag(Name) Name
#else
#define call(Func, ...) :: _##Func ( __VA_ARGS__ )
#define flag(Name) _##Name
#endif


struct open_flags
{
    bool inited = false;
 
    int o_append   = flag(O_APPEND);
    int o_creat    = flag(O_CREAT);
    int o_excl     = flag(O_EXCL);

#if defined (BOOST_POSIX_API)
    int o_noctty   = flag(O_NOCTTY);;
    int o_nonblock = flag(O_NONBLOCK);
    int o_sync     = flag(O_SYNC);
    int o_async    = flag(O_ASYNC);
    int o_cloexec  = flag(O_CLOEXEC);
    int o_direct   = flag(O_DIRECT);
    int o_directory= flag(O_DIRECTORY);
    int o_dsync    = flag(O_DSYNC);
    int o_largefile= flag(O_LARGEFILE);
    int o_noatime  = flag(O_NOATIME);
    int o_ndelay   = flag(O_NDELAY);
    int o_path     = flag(O_PATH);
#endif 
    int o_trunc    = flag(O_TRUNC);
    int o_rdonly   = flag(O_RDONLY);
    int o_wronly   = flag(O_WRONLY);
    int o_rdwr     = flag(O_RDWR);

    int s_iread  = flag(S_IREAD);
    int s_iwrite = flag(S_IWRITE);

    int s_irwxu = flag(S_IRWXU);
    int s_irusr = flag(S_IRUSR);
    int s_iwusr = flag(S_IWUSR);
    int s_ixusr = flag(S_IXUSR);
    
#if defined (BOOST_POSIX_API)
    int s_irwxg = flag(S_IRWXG);
    int s_irgrp = flag(S_IRGRP);
    int s_iwgrp = flag(S_IWGRP);
    int s_ixgrp = flag(S_IXGRP);
    int s_irwxo = flag(S_IRWXO);
    int s_iroth = flag(S_IROTH);
    int s_iwoth = flag(S_IWOTH);
    int s_ixoth = flag(S_IXOTH);
    int s_isuid = flag(S_ISUID);
    int s_isgid = flag(S_ISGID);
    int s_isvtx = flag(S_ISVTX);
#endif

    void load(frame & fr)
    {
        try { o_append   = std::stoi(fr.print("O_APPEND")   .value); } catch (metal::debug::interpreter_error&) {}
        try { o_creat    = std::stoi(fr.print("O_CREAT")    .value); } catch (metal::debug::interpreter_error&) {}
        try { o_excl     = std::stoi(fr.print("O_EXCL")     .value); } catch (metal::debug::interpreter_error&) {}
#if defined (BOOST_POSIX_API)
        try { o_noctty   = std::stoi(fr.print("O_NOCTTY")   .value); } catch (metal::debug::interpreter_error&) {}
        try { o_nonblock = std::stoi(fr.print("O_NONBLOCK") .value); } catch (metal::debug::interpreter_error&) {}
        try { o_sync     = std::stoi(fr.print("O_SYNC")     .value); } catch (metal::debug::interpreter_error&) {}
        try { o_async    = std::stoi(fr.print("O_ASYNC")    .value); } catch (metal::debug::interpreter_error&) {}
        try { o_cloexec  = std::stoi(fr.print("O_CLOEXEC")  .value); } catch (metal::debug::interpreter_error&) {}
        try { o_direct   = std::stoi(fr.print("O_DIRECT")   .value); } catch (metal::debug::interpreter_error&) {}
        try { o_directory= std::stoi(fr.print("O_DIRECTORY").value); } catch (metal::debug::interpreter_error&) {}
        try { o_dsync    = std::stoi(fr.print("O_DSYNC")    .value); } catch (metal::debug::interpreter_error&) {}
        try { o_largefile= std::stoi(fr.print("O_LARGEFILE").value); } catch (metal::debug::interpreter_error&) {}
        try { o_noatime  = std::stoi(fr.print("O_NOATIME")  .value); } catch (metal::debug::interpreter_error&) {}
        try { o_ndelay   = std::stoi(fr.print("O_NDELAY")   .value); } catch (metal::debug::interpreter_error&) {}
        try { o_path     = std::stoi(fr.print("O_PATH")     .value); } catch (metal::debug::interpreter_error&) {}
#endif
        try { o_trunc    = std::stoi(fr.print("O_TRUNC")   .value); } catch (metal::debug::interpreter_error&) {}
        try { o_rdonly   = std::stoi(fr.print("O_RDONLY")  .value); } catch (metal::debug::interpreter_error&) {}
        try { o_wronly   = std::stoi(fr.print("O_WRONLY")  .value); } catch (metal::debug::interpreter_error&) {}
        try { o_rdwr     = std::stoi(fr.print("O_RDWR")    .value); } catch (metal::debug::interpreter_error&) {}

        try { s_iread =  std::stoi(fr.print("S_IREAD"). value); } catch (metal::debug::interpreter_error&) {}
        try { s_iwrite = std::stoi(fr.print("S_IWRITE").value); } catch (metal::debug::interpreter_error&) {}
        
        try { s_iwusr = std::stoi(fr.print("S_IWUSR").value); } catch (metal::debug::interpreter_error&) {}
        try { s_ixusr = std::stoi(fr.print("S_IXUSR").value); } catch (metal::debug::interpreter_error&) {}
#if defined (BOOST_POSIX_API)
        try { s_irwxg = std::stoi(fr.print("S_IRWXG").value); } catch (metal::debug::interpreter_error&) {}
        try { s_irgrp = std::stoi(fr.print("S_IRGRP").value); } catch (metal::debug::interpreter_error&) {}
        try { s_iwgrp = std::stoi(fr.print("S_IWGRP").value); } catch (metal::debug::interpreter_error&) {}
        try { s_ixgrp = std::stoi(fr.print("S_IXGRP").value); } catch (metal::debug::interpreter_error&) {}
        try { s_irwxo = std::stoi(fr.print("S_IRWXO").value); } catch (metal::debug::interpreter_error&) {}
        try { s_iroth = std::stoi(fr.print("S_IROTH").value); } catch (metal::debug::interpreter_error&) {}
        try { s_iwoth = std::stoi(fr.print("S_IWOTH").value); } catch (metal::debug::interpreter_error&) {}
        try { s_ixoth = std::stoi(fr.print("S_IXOTH").value); } catch (metal::debug::interpreter_error&) {}
        try { s_isuid = std::stoi(fr.print("S_ISUID").value); } catch (metal::debug::interpreter_error&) {}
        try { s_isgid = std::stoi(fr.print("S_ISGID").value); } catch (metal::debug::interpreter_error&) {}
        try { s_isvtx = std::stoi(fr.print("S_ISVTX").value); } catch (metal::debug::interpreter_error&) {}
#endif
        inited = true;
    }
    int get_flags(int in)
    {
        int out = 0;
#if defined (BOOST_WINDOWS_API)
        out |= _O_BINARY;
#endif
        if (in & o_append  ) out |= flag(O_APPEND);
        if (in & o_creat   ) out |= flag(O_CREAT);
        if (in & o_excl    ) out |= flag(O_EXCL);
        if (in & o_rdonly  ) out |= flag(O_RDONLY);
        if (in & o_wronly  ) out |= flag(O_WRONLY);
        if (in & o_rdwr    ) out |= flag(O_RDWR);
#if defined (BOOST_POSIX_API)
        if (in & o_noctty   ) out |= flag(O_NOCTTY);
        if (in & o_nonblock ) out |= flag(O_NONBLOCK);
        if (in & o_sync     ) out |= flag(O_SYNC);
        if (in & o_async    ) out |= flag(O_ASYNC);
        if (in & o_cloexec  ) out |= flag(O_CLOEXEC);
        if (in & o_direct   ) out |= flag(O_DIRECT);
        if (in & o_directory) out |= flag(O_DIRECTORY);
        if (in & o_dsync    ) out |= flag(O_DSYNC);
        if (in & o_largefile) out |= flag(O_LARGEFILE);
        if (in & o_noatime  ) out |= flag(O_NOATIME);
        if (in & o_ndelay   ) out |= flag(O_NDELAY);
        if (in & o_path     ) out |= flag(O_PATH);
#endif
        if (in & o_trunc   ) out |= flag(O_TRUNC);

        return out;
    }

    int get_mode(int in)
    {
        int out = 0;
#if defined(BOOST_MSVC) || defined(BOOST_MSVC_FULL_VER)
        if (in & s_irwxu) out |= flag(S_IREAD) | flag(S_IWRITE);
        if (in & s_irusr) out |= flag(S_IREAD);
        if (in & s_iwusr) out |= flag(S_IWRITE);
#else
        if (in & s_iread) out |= flag(S_IREAD);
        if (in & s_iwrite)out |= flag(S_IWRITE);
        if (in & s_irwxu) out |= flag(S_IRWXU);
        if (in & s_irusr) out |= flag(S_IRUSR);
        if (in & s_iwusr) out |= flag(S_IWUSR);
        if (in & s_ixusr) out |= flag(S_IXUSR);
#endif
#if defined (BOOST_POSIX_API)
        if (in & s_irwxg) out |= flag(S_IRWXG);
        if (in & s_irgrp) out |= flag(S_IRGRP);
        if (in & s_iwgrp) out |= flag(S_IWGRP);
        if (in & s_ixgrp) out |= flag(S_IXGRP);
        if (in & s_irwxo) out |= flag(S_IRWXO);
        if (in & s_iroth) out |= flag(S_IROTH);
        if (in & s_iwoth) out |= flag(S_IWOTH);
        if (in & s_ixoth) out |= flag(S_IXOTH);
        if (in & s_isuid) out |= flag(S_ISUID);
        if (in & s_isgid) out |= flag(S_ISGID);
        if (in & s_isvtx) out |= flag(S_ISVTX);
#endif
        return out;
    }

};

struct seek_flags
{
    bool inited = false;


    int seek_set = SEEK_SET;
    int seek_cur = SEEK_CUR;
    int seek_end = SEEK_END;

    void load(frame & fr)
    {
        try { seek_set = std::stoi(fr.print("SEEK_SET").value); } catch (metal::debug::interpreter_error&) {}
        try { seek_cur = std::stoi(fr.print("SEEK_CUR").value); } catch (metal::debug::interpreter_error&) {}
        try { seek_end = std::stoi(fr.print("SEEK_END").value); } catch (metal::debug::interpreter_error&) {}
        inited = true;
    }
    int get_flags(int in)
    {
        int out = 0;

        if (in & seek_set ) out |= SEEK_SET;
        if (in & seek_cur ) out |= SEEK_CUR;
        if (in & seek_end ) out |= SEEK_END;

        return out;
    }
};


struct metal_func_stub : break_point
{
    metal_func_stub() : break_point("metal_func_stub")
    {
    }

    void invoke(frame & fr, const std::string & file, int line) override
    {
        auto type = fr.arg_list().at(0);
        if (type.id != "func_type")
            return;

        if (type.value == "metal_func_close")
            this->close(fr);
        else if (type.value == "metal_func_fstat")
            this->fstat(fr);
        else if (type.value == "metal_func_isatty")
            this->isatty(fr);
        else if (type.value == "metal_func_link")
            this->link(fr);
        else if (type.value == "metal_func_lseek")
            this->lseek(fr);
        else if (type.value == "metal_func_open")
            this->open(fr);
        else if (type.value == "metal_func_read")
            this->read(fr);
        else if (type.value == "metal_func_stat")
            this->stat(fr);
        else if (type.value == "metal_func_symlink")
            this->symlink(fr);
        else if (type.value == "metal_func_unlink")
            this->unlink(fr);
        else if (type.value == "metal_func_write")
            this->write(fr);
    }
    void close(frame & fr)
    {
        auto fd = std::stoi(fr.arg_list().at(3).value);
        auto ret = call(close, fd);

        fr.log() << "***metal_newlib*** Log: Invoking close(" << fd << ") -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }
    void fstat(frame & fr)
    {
        auto fd = std::stoi(fr.arg_list().at(3).value);

#if defined (BOOST_WINDOWS_API)
 #if defined (_WIN64)
        struct _stat64i32 st;
 #else
        struct _stat st;
 #endif
#else
        struct stat st;
#endif
        int ret = call(fstat, fd, &st);

        fr.set("arg7->st_dev",   std::to_string(st.st_dev));
        fr.set("arg7->st_ino",   std::to_string(st.st_ino));
        fr.set("arg7->st_mode",  std::to_string(st.st_mode));
        fr.set("arg7->st_nlink", std::to_string(st.st_nlink));
        fr.set("arg7->st_uid",   std::to_string(st.st_uid));
        fr.set("arg7->st_gid",   std::to_string(st.st_gid));
        fr.set("arg7->st_rdev",  std::to_string(st.st_rdev));
        fr.set("arg7->st_size",  std::to_string(st.st_size));
        fr.set("arg7->st_atime", std::to_string(st.st_atime));
        fr.set("arg7->st_mtime", std::to_string(st.st_mtime));
        fr.set("arg7->st_ctime", std::to_string(st.st_ctime));

        fr.log() << "***metal_newlib*** Log: Invoking fstat(" << fd << ", **local pointer**) -> " << ret << std::endl;


        fr.return_(std::to_string(ret));
    }

    void isatty(frame & fr)
    {
        auto fd = std::stoi(fr.arg_list().at(3).value);
        auto ret = call(isatty, fd);

        fr.log() << "***metal_newlib*** Log: Invoking isatty(" << fd << ") -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }

    void link(frame & fr)
    {
        auto existing = fr.get_cstring(1);
        auto _new     = fr.get_cstring(2);
#if defined (BOOST_POSIX_API)
        auto ret = ::link(existing.c_str(), _new.c_str());
#else
        int ret = 0;
        if (!CreateHardLinkA(_new.c_str(), existing.c_str(), nullptr))
            ret = GetLastError();
#endif

        fr.log() << "***metal_newlib*** Log: Invoking link(" << existing << ", " << _new <<  ") -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }

    seek_flags sf;

    void lseek(frame & fr)
    {
        auto fd  = std::stoi(fr.arg_list().at(3).value);
        auto ptr = std::stoi(fr.arg_list().at(4).value);
        auto dir_in = std::stoi(fr.arg_list().at(5).value);

        if (!sf.inited)
            sf.load(fr);

        auto dir = sf.get_flags(dir_in);
        auto ret = call(lseek,fd, ptr, dir);

        fr.log() << "***metal_newlib*** Log: Invoking lseek(" << fd << ", " << ptr << ", " << dir_in << ") -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }

    open_flags of;

    void open(frame & fr)
    {
        auto file  = fr.get_cstring(1);
        auto flags_in = std::stoi(fr.arg_list().at(3).value);
        auto mode_in  = std::stoi(fr.arg_list().at(4).value);

#if defined(BOOST_POSIX_API)
        boost::algorithm::replace_all(file, "\\", "/");
        boost::algorithm::replace_all(file, "//", "/");
        boost::algorithm::replace_all(file, "//", "/");
#else
        boost::algorithm::replace_all(file, "/", "\\");
        boost::algorithm::replace_all(file, "\\\\", "\\");
        boost::algorithm::replace_all(file, "\\\\", "\\");
#endif 

        if (!of.inited)
            of.load(fr);

        auto flags = of.get_flags(flags_in);
        auto mode  = of.get_mode (mode_in);

        auto ret = call(open, file.c_str(), flags, mode);
        
        fr.log() << std::oct;
        fr.log() << "***metal_newlib*** Log: Invoking open(\"" << file << "\", 0" << flags << ", 0" << mode << ") -> " << ret << std::endl;
        fr.log() << std::dec;
        fr.return_(std::to_string(ret));
    }

    void read(frame & fr)
    {
        auto fd  = std::stoi(fr.arg_list().at(3).value);
        auto len = std::stoi(fr.arg_list().at(4).value);
        auto ptr = std::stoull(fr.arg_list(6).value, nullptr, 16);
        std::vector<std::uint8_t> buf(len, static_cast<char>(0));

        auto ret = call(read, fd, buf.data(), len);
        if (len > 0) //zero can only check if it's open.
            fr.write_memory(ptr, buf);

        fr.log() << "***metal_newlib*** Log: Invoking read(" << fd << ", ***local pointer***, " << len << ") -> " << ret << std::endl;
        fr.return_(std::to_string(ret));
    }

    void stat(frame & fr)
    {
        auto file = fr.arg_list().at(1).value;
#if defined (BOOST_WINDOWS_API)
 #if defined (_WIN64)
        struct _stat64i32 st;
 #else
        struct _stat st;
 #endif
#else
        struct stat st;
#endif

        int ret = call(stat, file.c_str(), &st);

        fr.set("arg7->st_dev",   std::to_string(st.st_dev));
        fr.set("arg7->st_ino",   std::to_string(st.st_ino));
        fr.set("arg7->st_mode",  std::to_string(st.st_mode));
        fr.set("arg7->st_nlink", std::to_string(st.st_nlink));
        fr.set("arg7->st_uid",   std::to_string(st.st_uid));
        fr.set("arg7->st_gid",   std::to_string(st.st_gid));
        fr.set("arg7->st_rdev",  std::to_string(st.st_rdev));
        fr.set("arg7->st_size",  std::to_string(st.st_size));
        fr.set("arg7->st_atime", std::to_string(st.st_atime));
        fr.set("arg7->st_mtime", std::to_string(st.st_mtime));
        fr.set("arg7->st_ctime", std::to_string(st.st_ctime));

        fr.log() << "***metal_newlib*** Log: Invoking stat(\"" << file << "\", ***local pointer***) -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }

    void symlink(frame & fr)
    {
        auto existing = fr.get_cstring(1);
        auto _new     = fr.get_cstring(2);
#if defined (BOOST_POSIX_API)
        auto ret = ::symlink(existing.c_str(), _new.c_str());
#else
        int ret = 0;
#if defined(CreateSymbolicLink)
        //BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_VISTA
        if (!CreateSymbolicLinkA(_new.c_str(), existing.c_str(), 0x0))
            ret = GetLastError();
#else
        if (!CreateHardLinkA(_new.c_str(), existing.c_str(), 0x0))
            ret = GetLastError();
#endif
#endif

        fr.log() << "***metal_newlib*** Log: Invoking symlink(" << existing << ", " << _new <<  ") -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }

    void unlink(frame & fr)
    {
        auto name = fr.get_cstring(6);
#if defined (BOOST_POSIX_API)
        auto ret = ::unlink(name.c_str());
#else
        int ret = 0;
        if (!DeleteFileA(name.c_str()))
            ret = GetLastError();
#endif
        fr.log() << "***metal_newlib*** Log: Invoking unlink(" << name << ") -> " << ret << std::endl;
        fr.return_(std::to_string(ret));
    }

    void write(frame & fr)
    {
        auto fd  = std::stoi(fr.arg_list().at(3).value);
        auto len = std::stoi(fr.arg_list().at(4).value);
        auto ptr = std::stoull(fr.arg_list(6).value, nullptr, 16);
    
        auto data = fr.read_memory(ptr, len);
        auto ret = call(write, fd, data.data(), len);

        fr.log() << "***metal_newlib*** Log: Invoking write(" << fd << ", ***local pointer***, " << len << ") -> " << ret << std::endl;

        fr.return_(std::to_string(ret));
    }
};

void metal_dbg_setup_bps(std::vector<std::unique_ptr<metal::debug::break_point>> & bps)
{
    bps.push_back(std::make_unique<metal_func_stub>());
};


