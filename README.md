[![status](https://travis-ci.org/klemens-morgenstern/metal.test.svg?branch=master)](https://travis-ci.org/klemens-morgenstern/metal.test) ![stable](https://img.shields.io/badge/stability-stable_beta-green.svg) [![Issues](https://img.shields.io/github/issues/klemens-morgenstern/metal.test.svg)](https://github.com/klemens-morgenstern/metal.test/issues)

# About

This framework provides facilities for automated execution of remote code. The core idea is, to utilize the debugger for automated testing, so that it's featured can be used to obtain information.

At the current state it provides functionality for:
 
 * I/O Forwarding
 * Code Coverage
 * Unit testing
 * Call tracing
 * Profiling
 * Function Stubbing at link-time

This is based on plugin-system, so that it can be extended with functionality for your system. The above features are also done by plugins that are provided by the framework.
Since it is built do use the debugger, it can be used for every case a debugger can, which includes the following applications:

 * local execution
 * remote server (*gdbsever*)
 * [openocd](http://openocd.org/)
 * [qemu](http://www.qemu.org/)

When used with the right plugins, an application run via openocd or qemu behaves as if it were run locally and can be integrated into any automated testing.

*It currently only supports the gdb, but lldb support is planned for a future version.*

# Tool Overview

## [metal.runner](https://github.com/klemens-morgenstern/metal.test/wiki/runner.introduction)

The debug-runner is the core module of the toolest, it allows automated execution of the debugger. This can be used with plugins, so that custom functionality can be added to breakpoints. The main application is for the automated executing tests on remote targets, such as embedded platforms.
It comes with two plugins for embedded targets:

 - I/O (similar to semi-hosting)
 - Exit-Code 
 
When used with these two plugins an embedded application can be run on target, but behaves as if it were run locally. In addition calltrace & backend provide part of the functionality as part as metal.runner plugins.

## [metal.unit](https://github.com/klemens-morgenstern/metal.test/wiki/backend.introduction)

We provide a very light weight test backend, for easy use with our `metal.runner`. When used without the debugger, it will only yield a binary result, while it will provide very detailed information through the debugger.
This means that it can be used with the same assertions 

## [metal.calltrace](https://github.com/klemens-morgenstern/metal.test/wiki/calltrace.introduction)

The calltrace provides a way to assert a certain call sequence for functions. Combined with the `metal.runner` it can be used to log function calls and add profiling.


## [metal.serial](https://github.com/klemens-morgenstern/metal.test/wiki/serial.introduction)

The serial library provides a light-weight testing tool for environments that do not provide access for a debugger.
It outputs the minimum amount of data from the target device, reads the rest from the source files and provides more detailed report that way.
E.g. it provides a printf-alternative that does the actual formatting on the host side, thus providing formatted output without required heap usage on the target.


# Documentation

The current master Documentation can be found in the [wiki](https://github.com/klemens-morgenstern/metal.test/wiki).

### Test results

Branches        | Build         | Tests coverage | 
----------------|-------------- | -------------- |
Develop:        | [![Build Status](https://travis-ci.org/klemens-morgenstern/metal.test.svg?branch=develop)](https://travis-ci.org/klemens-morgenstern/metal.test) | [![Coverage Status](https://coveralls.io/repos/github/klemens-morgenstern/metal.test/badge.svg?branch=develop)](https://coveralls.io/github/klemens-morgenstern/metal.test?branch=develop) |
Master:         | [![Build Status](https://travis-ci.org/klemens-morgenstern/metal.test.svg?branch=master)](https://travis-ci.org/klemens-morgenstern/metal.test)  | [![Coverage Status](https://coveralls.io/repos/github/klemens-morgenstern/metal.test/badge.svg?branch=master)](https://coveralls.io/github/klemens-morgenstern/metal.test?branch=master)   |

# Dependency

This library requires boost 1.64.

