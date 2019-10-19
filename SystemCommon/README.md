# SystemCommon

A collection of useful system-level utilities

## Shared Components

some utilities that are too basic to find a suitable place to host.

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

## System Components

Some utilities aims to provide equal functionality on different OSs.

### [FileEx](./FileEx.h)

Provide stream for files. FileStreams are acquired from `FileObject` which uses RAII to wrap file handle.

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

[MiniLogger](../MiniLogger)'s `ConsoleBackend` is based on this.

### [ThreadEx](./ThreadEx.h)

A wrapper to support setting or getting thread's information. It's designed to be cross-platform but not fully tested.

## License

common (including its component) is licensed under the [MIT license](../License.txt).