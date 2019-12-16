# Twarlock

This is a stand-alone program which can interact with Twitch in various ways.
For example, the `info` command looks up general information about a Twitch
channel.

Twarlock uses the `Http::Client` class along with the
`HttpNetworkTransport::HttpClientNetworkTransport` and
`TlsDecorator::TlsDecorator` classes in order to access Twitch APIs.

## Usage

    Usage: Twarlock [-c <CFG>] <CMD> [ARG]..

    Execute the given command.

        CFG  Path to file containing the program configuration If not specified,
             Twarlock searches for a configuration file named 'Twarlock.json' in
             the current working directory, and then 'Twarlock.json' in directory
             containing the program, and then '.twarlock' the current user's home
             directory.

        CMD  Name of command to execute:
             info  Query channel and user information


    Usage: Twarlock -h <CMD>

    Print usage information about a specific command and exit.

        CMD  Name of command for which to get more information

`Twarlock` requires a configuration file in JSON format, for information
needed by various commands, along with commonly-needed information such
as the Twitch app Client ID to use in API requests.

## Supported platforms / recommended toolchains

`Twarlock` is a portable C++11 application which depends only on the
C++11 compiler, the C and C++ standard libraries, and other C++11 libraries
with similar dependencies, so it should be supported on almost any platform.
The following are recommended toolchains for popular platforms.

* Windows -- [Visual Studio](https://www.visualstudio.com/) (Microsoft Visual
  C++)
* Linux -- clang or gcc
* MacOS -- Xcode (clang)

## Building

There are two distinct steps in the build process:

1. Generation of the build system, using CMake
2. Compiling, linking, etc., using CMake-compatible toolchain

### Prerequisites

* [AsyncData](https://github.com/rhymu8354/AsyncData.gie) - a library
  containing classes and templates designed to support asynchronous and
  lock-free communication and data exchange between different components in a
  larger system.
* [CMake](https://cmake.org/) version 3.8 or newer
* C++11 toolchain compatible with CMake for your development platform (e.g.
  [Visual Studio](https://www.visualstudio.com/) on Windows)
* [Http](https://github.com/rhymu8354/Http.git) - a library which implements
  [RFC 7230](https://tools.ietf.org/html/rfc7230), "Hypertext Transfer Protocol
  (HTTP/1.1): Message Syntax and Routing".
* [HttpNetworkTransport](https://github.com/rhymu8354/HttpNetworkTransport.git) -
  a library which implements the transport interfaces needed by the `Http`
  library, in terms of the network endpoint and connection abstractions
  provided by the `SystemAbstractions` library.
* [Json](https://github.com/rhymu8354/Json.git) - a library which implements
  [RFC 7159](https://tools.ietf.org/html/rfc7159), "The JavaScript Object
  Notation (JSON) Data Interchange Format".
* [StringExtensions](https://github.com/rhymu8354/StringExtensions.git) - a
  library containing C++ string-oriented libraries, many of which ought to be
  in the standard library, but aren't.
* [SystemAbstractions](https://github.com/rhymu8354/SystemAbstractions.git) - a
  cross-platform adapter library for system services whose APIs vary from one
  operating system to another
* [TlsDecorator](https://github.com/rhymu8354/TlsDecorator.git) - an adapter to
  use `LibreSSL` to encrypt traffic passing through a network connection
  provided by `SystemAbstractions`

### Build system generation

Generate the build system using [CMake](https://cmake.org/) from the solution
root.  For example:

```bash
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A "x64" ..
```

### Compiling, linking, et cetera

Either use [CMake](https://cmake.org/) or your toolchain's IDE to build.
For [CMake](https://cmake.org/):

```bash
cd build
cmake --build . --config Release
```
