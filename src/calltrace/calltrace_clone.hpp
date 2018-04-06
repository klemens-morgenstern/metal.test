/**
 * @file   src/calltrace_clone.hpp
 * @date   08.05.2017
 * @author Klemens D. Morgenstern
 *

 */

#include <metal/debug/frame.hpp>

#include <iostream>

#ifndef CALLTRACE_CLONE_HPP_
#define CALLTRACE_CLONE_HPP_

struct calltrace_clone_entry
{
    std::uint64_t address{0};
    boost::optional<metal::debug::address_info> info;

    calltrace_clone_entry() = default;
    calltrace_clone_entry(std::uint64_t address, boost::optional<metal::debug::address_info> && info)
        : address(address), info(std::move(info)) {}
    calltrace_clone_entry(calltrace_clone_entry &&) = default;
    calltrace_clone_entry(const calltrace_clone_entry &) = default;

    calltrace_clone_entry& operator=(calltrace_clone_entry &&) = default;
    calltrace_clone_entry& operator=(const calltrace_clone_entry &) = default;
};

class calltrace_clone
{
    std::uint64_t _location;
    calltrace_clone_entry _fn;

    std::vector<calltrace_clone_entry> _content;

    int _repeat;
    int _skip;

    int _to_skip = _skip;
    int _errors = 0;
    int _current_position = 0;
    int _start_depth = -1;
    int _repeated = 0;

    bool _ovl = false;
public:
    calltrace_clone(std::uint64_t location,
                    calltrace_clone_entry && fn,
                    std::vector<calltrace_clone_entry> && content,
                    int repeat, int skip)
                : _location(location),
                  _fn(std::move(fn)),
                  _content(std::move(content)),
                  _repeat(repeat), _skip(skip) {};

    calltrace_clone(const calltrace_clone &) = default;
    calltrace_clone(calltrace_clone &&) = default;

    calltrace_clone& operator=(const calltrace_clone &) = default;
    calltrace_clone& operator=(calltrace_clone &&) = default;

    const calltrace_clone_entry & fn() const {return _fn;}
    const std::vector<calltrace_clone_entry> & content() const {return _content;}
    int repeat() const {return _repeat;}
    int skip()   const {return _skip;}

    bool inactive() const {return _start_depth == -1;}
    int  repeated() const {return _repeated;}

    std::uint64_t location() const { return _location;}

    bool to_skip() const {return _to_skip > 0;}
    void add_skipped() {_to_skip--;}

    void start(int depth) {_start_depth = depth; _ovl = false;}

    bool my_depth(int value) const {return value == _start_depth;}
    bool my_child(int value) const {return value == (_start_depth + 1);}

    int errors() const {return _errors;}
    int current_position() const { return _current_position;}

    const calltrace_clone_entry previous() const 
    {
        if ((_current_position <= _content.size()) && (_current_position > 0))
            return _content[_current_position - 1];
        else
            return {0, boost::none};
    }
    
    
    bool check(std::uint64_t this_fn)
    {
        if (_current_position >= static_cast<int>(_content.size()))
        {
            _ovl = true;
            _errors++;
            return false;
        }
        auto ptr = _content[_current_position].address;
        _current_position++;
        if ((ptr != this_fn) && (ptr != 0))
        {
            _errors++;
            return false;
        }
        else
            return true;
    }

    bool ovl() const {return _ovl;}
    bool stop()
    {
        _start_depth = -1;
        bool res = true;
        if (_current_position != static_cast<int>(_content.size()))
        {
            _errors++;
            res = false;
        }
        _current_position = 0;
        _repeated++;

        return res;
    }
};



#endif /* CALLTRACE_CLONE_HPP_ */
