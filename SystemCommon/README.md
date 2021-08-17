# SystemCommon

A collection of useful system-level utilities

## Shared Components

some utilities that are too basic to find a suitable place to host.

### [RuntimeFastPath](./RuntimeFastPath.h)

Common infrastructure to provide runtime-decided fastpath for some operations.

### [LoopBase](./LoopBase.h)

* `LoopBase`    Base structure for loop based operation.

* `LoopExecutor`    Base structure that runs a loop, handles the majority work.

They provide loop based operation support, with loop implememntation and execution seperated. Only polling-like scheduler supported by dead-loop.

There' 2 kind of Executor:

* Threaded: runs the loop in a new thread, with proper synchronization.
* Inplace:  runs the loop in a given thread, with no sleep support.

#### Flash Point

In order to reduce overhead of sleep/wait and lock, LoopBase uses a **"4-state"** sleep strategy.

```
                                     |========|
Others wakeup prevent sleeping    -> | Forbid | --|
                                 /   |========|   |
                                /                 |
|=========|       |==========| /     |========|   |
| Running | ----> | Prending | ----> | Sleep  | --| Others   really wait  and notify
|=========|       |==========|       |========|   | Executor really sleep
     ^                                            |
     |--------------------------------------------|
```

### [PromiseTask](./PromiseTask.h)

`PromiseResut` is the foundation of other async operation utilities (like `AsyncExecutor`, `OpenGLUtil`, `OpenCLUtil`), which provides a common interface to operate a result which may not be ready yet.

Non-templated `PromiseResultCore` provides state and time infomation, which ease the promise storing with type-erasure. Because of this, `PromiseResult` should be stored in heap and wrapped by `shared_ptr`.

`IsPromiseResult<T>()` and `EnsurePromiseResult<T>()` can be used to check if `T` is type of `PromiseResult`. `PromiseChecker` provides ability to return result type.

[PromiseTaskSTD.h](PromiseTaskSTD.h) is a wrapper for C++11's `future` and `promise`. It is seperated due to the incompatiblility with C++/CLI.

### [MiniLogger](./MiniLogger.h)

MiniLogger is a simple logger that provide thread-safe(maybe) logging support and global logging management.

Logger is separated as frontend and backend. Both sync/async backend are supported, but async backends are preferred.

Log content formation and message dispatcher are handled by frontend, which is almost "stateless" and runs at caller's thread.

Logging operations are handled by backend, which could be on another thread if LoggerQBackend(Queue based async backend) are used.

Backend are bound with logger instance, but they are "shared". Also, logger has a static backend, running on an isolated thread, accepting global callback bindings.

#### Backend

Backends are supported with `LoopBase`.

* **Console Backend** `shared`
  
  Windows only, support colorful output.

* **Debugger Backend** `shared`
  
  Send debug message to debugger, such as VisualStudio's debug window, or stderr in Linux

* **File Backend** `not-shared`
  
  Simply write log to file

* **Global Backend** `shared`
  
  Global hook. Expose ability to capture logs in other runtime (.Net).

### [CopyEx](./CopyEx.h) 

Based on `RuntimeFastPath`.

A collection of batch operations for type-conversion with acceleration of [SIMD](../common/simd). 

### [MiscIntrins](./MiscIntrins.h)

Based on `RuntimeFastPath`.

A wrapper of some basic operations that may be supported by using CPU intrins. 

### [DigestFuncs](./MiscIntrins.h)

Based on `RuntimeFastPath`.

A wrapper of some digest functions that may be supported by using CPU intrins.

* `SHA256`: 
  * `SHA-NI`, `SHA-NI+AVX2` on x86.
  * `ARMv8-SHA2` on Arm.
  * `NAIVE` on all based on [`digestpp`](../3rdParty/digestpp)

## System Components

Some utilities aims to provide equal functionality on different OSs.

### [FileEx](./FileEx.h)

Provide stream for files. FileStreams are acquired from `FileObject` which uses RAII to wrap file handle.

According to the common interface([`FileBase.hpp`](../common/FileBase.hpp)), it will try to use `filesysten` from STL if possibile. If is not supported, or explicit requested using "COMMON_FS_USE_GHC", it will switch to use [`ghc-filesystem`](https://github.com/gulrak/filesystem).
It's not using `boost::filesystem` because boost does not support `char16_t` & `char32_t`.

### [RawFileEx](./RawFileEx.h)

Provide native stream for files. RawFileStreams are acquired from `RawFileObject` which uses RAII to wrap raw file handle.

Compared to [`FileEx`](./FileEx.h), it supports more native property like `non-buffering` or `share-access`, it can also be used to create filemapping.

### [FileMapperEx](./FileMapperEx.h)

Provide basic file mapping support with native file resources. `FileMappingStream` are basically MemoryStream, while it tries to handle flush.

**Not fully tested**.

### [ConsoleEx](./ConsoleEx.h)

Provide console-related operation, like quering console size. It also provides `getch`, `getche` for linux using `termios`.

### [ColorConsole](./ColorConsole.h)

Provide color support for console. For Win32 which doesn't support VT mode, it emulating it using `SetConsoleTextAttribute`.

[MiniLogger](./MiniLogger.cpp)'s `ConsoleBackend` is based on this.

### [ThreadEx](./ThreadEx.h)

A wrapper to support setting or getting thread's information. It's designed to be cross-platform but not fully tested.

## Dependency

* `readline` a GNU Readline library provides a set of functions for use by applications that allow users to edit command lines as they are typed in.

  `ConsoleEx`'s `ReadLine` depends on it when on *nix.

  The library dynamically link to it, so you need to install `readline` on both the Dev OS and Host OS. E.g, `libreadline-dev` and `libreadlineX` on ubuntu, where `X` depends on dist version (`7` for 18.04 and `8` for 20.04).

* `termios` the newer Unix API for terminal I/O.
  
  `ConsoleEx`'s `ReadCharImmediate` depends on it when on *nix.
  
  Most *nix dist should be able to use it in box.

* [`digestpp`](../3rdParty/digestpp) [C++11 header-only message digest library](https://github.com/kerukuro/digestpp) ([Public Domain](../3rdParty/digestpp/LICENSE))

  `DigestFuncs` depens on it to provide NAIVE implementation.

## License

common (including its component) is licensed under the [MIT license](../License.txt).