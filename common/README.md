# common

A collection of useful utilities

## Componoent

* **AlignedContainer**

  A custom allocator that support aligned memory managment

* **ContainerEx**

  A simple library providing some shortcuts for container operations

* **DelayLoader**

  A wrapper to manage win32's delay-laod DLL

* **Exception**

  Custom Exception model, inherit from std::runtime_error, with support of nested-exception, strong-type, unicode message, arbitray extra data...

* **FileEx**

  A wrapper of file operations, using RAII to wrap file handle. Buffered reader supported using CRTP.

* **FileMapper** `buggy`

  A wrapper to manage win32's file mapper

* **Linq** `buggy`

  A try of implementing linq on C++. It's unfinished and buggy

* **PromiseTask**

  A wrapper for C++11's future&promise for C++/CLI project. 
  
  It uses virtual method to hide STL's `thread`, which is not supported in C++/CLI.

  It also provides universial API for [AsyncExecutor](./AsyncExecutor)

* **ResourceHelper**

  A wrapper to manage win32 DLL's embeded resource

* **SharedResource**

  An auto management for shared resource.
  
  Some resources should be shared like static member or global variable, which make it hard to trace their lifetime.

  SharedResource is based on `Wrapper`, using weak_ptr and shared_ptr to managed resource. It is thread-safe and resource will get released once it's no longer held by anyone.

* **SpinLocker**

  Spin-lock implemented using std::stomic. It also provided a read-write lock(both priority supported) and a prefer-lock, both are spin-locked.

* **StringEx**

  Some useful utils for string operations like split, concat...

* **StrCharset**
  
  Define text charset and conversions between them. Conversion is self-made, partial optimized, and partial-checked.

  Thanks to `伐木丁丁鸟鸣嘤嘤`'s [GB18030-Unicode LUT](http://www.fmddlmyy.cn/text30.html), which is based on `谢振斌`'s work.
  LUT_gb18030.tab is based on their works.

* **TimeUtil**

  A util to manage time. It's mainly used to be a timer.

* **ThreadEx**

  A wrapper to support some operation in C++/CLI. It's designed to be cross-platform but only windows-platform are implemented using Win32 API.

* **Wrapper**

  An extended version of STL's shared_ptr. It adds some sugar to cover some design of shared_ptr, while it also leads to some problem.

* [**miniLogger**](./miniLogger)

  A simple logger that provide thread-safe(maybe) logging and global logging management.

  It uses [fmt](../3rdParty/fmt) as format support, with utf-support from [StrCharset](./StrCharset.hpp) (onyl new version).

* [**AsyncExecutor**](./AsyncExecutor)

  A async task-execution environment. It uses [boost.context](../3rdParty/boost.context) as backend support, providing async promise-waiting via PromiseTask.

  It simply uses a polling scheduler, waiting events are queried every xxms(default 20ms).

## Dependency

* C++17 required
  * `chrono` -- TimeUtil
  * `memory` -- shared_ptr and weak_ptr
  * `tuple` -- used in some container operation
  * `atomic` `mutex` `condition_variable` -- providing thread-safe operation
  * `thread` -- used to provide thread creation and sleep function
  * `future` -- provide STD PromiseTask
  * `any` -- used for Exception to carry arbitray data
  * `optional` -- providing "nullable" result

## Feature

Some components (\*.hpp) are header-only, while some (\*.h&\*.inl) are not. (\*.h) provides declaration and (\*.inl) provides implements. 

### SharedResource

Some resources need to be shared among classes or instances, hence they are often defined as "static".
"static" variable will be initialized when the first time it's reached, however, there is no promise when they will be destroyed.

Shared_ptr can be used to automatically manage resource since the inner resource will be automatically destroyed when the last one release it. This provide the missing part of "static".

SharedResource is designed to provide a determined lifespan for a shared resource.
The resource will be gennerated only someone ask for it, and will be destroyed when it is no longer held by anyone.
If there's another ask for resource after it destroy it, it will generate the resource again.
However, since itself should be defined as static, its' lifespan can not be promised.

Though shared_ptr itself has provided thread-safety promise, SharedResource itself should also manually provide it, since serveral thread may ask for resource at the same time, while only one resource should be generated.
I brutely use a spin-lock, which may lead to a disaster when multiple threads trying to acquire the resource that hasn't been generated --- they will be blocked and spin, causing high CPU usage.
Also, it uses weak_ptr to keep track of resource, whose lock method will cause some overhead.

The best practise should be acquiring the resource only once, and keep a copy of it until finish using it.

### Wrapper

Wrapper can be simply regarded as a combination of shared_ptr and make_shared. It is mostly my own taste and may not be recommanded.

Inside Wrapper I used some SFINAE tech to detect type, which may result in some compiler error --- SFINAE need class type fully defined, so you just can't use Wrapper of self in self's declaration.

### StringEx

`split` is simply based on brute find, and there's no optimized implements like KMP or SSE4.2 intrin.

`concat` uses C++11's variadic template to detect string's length and pre-alloc memory. I am not sure if recursive func calling would be inlined, so it may in fact hurt performance. Anyway, it's just a toy.

### StrCharset

StrCharset provide encoding defines and charset transform with self-made conversion class. Correctness check is not completed.

`codecvt` is removed since it is marked deprecated in C++17 and some conversion seems to be locale-dependent.

Converting encoding need to specify input charset, while StrCharset does not provide encoding-detection. If you need it , you should inlude [uchardet](../3rdParty/uchardetlib).


## License

common (including its component) is licensed under the [MIT license](../License.txt).