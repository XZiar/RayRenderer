# common

A collection of useful utilities

## Component

* **AlignedContainer**

  A custom allocator that support aligned memory management

* **ColorConsole**

  A library providing color support for console. For Win32 which doesn't support VT mode, it emulating it using `SetConsoleTextAttribute`.

* **ContainerEx**

  A simple library providing some shortcuts for container operations

* **Controllable**

  A base class using type erasure to support dynamic property access. object's property need to be registered explicitly.

  **Controllable stores data inside the instance, so multi-inherited class may consider using virtual inheritance.**

* **DelayLoader**

  A wrapper to manage win32's delay-load DLL

* **EasierJSON**

  A wrapper for `rapidjson`, providing strong-type json component and easier set/get conversion and operations.

* **Exception**

  Custom Exception model, inherit from std::runtime_error, with support of nested-exception, strong-type, Unicode message, arbitrary extra data...

* **FileEx**

  A wrapper of file operations, using RAII to wrap file handle. Buffered reader supported using CRTP.

* **FileMapper** `buggy`

  A wrapper to manage win32's file mapper

* **Linq**

  A try of implementing LINQ on C++.

* **PromiseTask**

  A wrapper for C++11's future&promise, provides universal API with preparation and time-query support. 
  
  It uses virtual method to hide STL's `thread` (which is not supported in C++/CLI).

* **ResourceHelper**

  A wrapper to manage embedded resource. It uses DLL's embedded resource on Win32 and binary object on *nix.

* **SharedResource**

  An auto management for shared resource.
  
  Some resources should be shared like static member or global variable, which make it hard to trace their lifetime.

  SharedResource is based on `Wrapper`, using weak_ptr and shared_ptr to managed resource. It is thread-safe and resource will get released once it's no longer held by anyone.

* **SharedString**

  An shared immutable string.
  
  Some string is used across threads but not being modified, so they can be shared with proper lifetime management.

  ShareString provides access using string_view and uses embedded reference-block to save pointer's dereference overhead.

* **StrCharset**

  StrCharset provide encoding defines and charset transform with self-made conversion class. Correctness check is not completed.

  `codecvt` is removed since it is marked deprecated in C++17 and some conversion seems to be locale-dependent.

  Converting encoding need to specify input charset, while StrCharset does not provide encoding-detection. If you need it , you should include [uchardet](../3rdParty/uchardetlib).

* **SpinLocker**

  Spin-lock implemented using std::atomic. It also provided a read-write lock(both priority supported) and a prefer-lock, both are spin-locked.

* **StringEx**

  Some useful utilities for string operations like split, concat...

* **StrCharset**
  
  Define text charset and conversions between them. Conversion is self-made, partial optimized, and partial-checked.

  Thanks to `伐木丁丁鸟鸣嘤嘤`'s [GB18030-Unicode LUT](http://www.fmddlmyy.cn/text30.html), which is based on `谢振斌`'s work.
  LUT_gb18030.tab is based on their works.

* **TimeUtil**

  A utility to provide time query support, mainly used as a timer.

* **ThreadEx**

  A wrapper to support setting or getting thread's information. It's designed to be cross-platform but not fully tested.

* **Wrapper**

  An extended version of STL's shared_ptr. It adds some sugar to cover some design of shared_ptr, while it also leads to some problem.

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
The resource will be generated only someone ask for it, and will be destroyed when it is no longer held by anyone.
If there's another ask for resource after it destroy it, it will generate the resource again.
However, since itself should be defined as static, its' lifespan can not be promised.

Though shared_ptr itself has provided thread-safety promise, SharedResource itself should also manually provide it, since several thread may ask for resource at the same time, while only one resource should be generated.
I brutally use a spin-lock, which may lead to a disaster when multiple threads trying to acquire the resource that hasn't been generated --- they will be blocked and spin, causing high CPU usage.
Also, it uses weak_ptr to keep track of resource, whose lock method will cause some overhead.

The best practise should be acquiring the resource only once, and keep a copy of it until finish using it.

### Wrapper

Wrapper can be simply regarded as a combination of shared_ptr and make_shared. It is mostly my own taste and may not be recommended.

Inside Wrapper I used some SFINAE tech to detect type, which may result in some compiler error --- SFINAE need class type fully defined, so you just can't use Wrapper of self in self's declaration.

### StringEx

`split` is simply based on brute find, and there's no optimized implements like KMP or SSE4.2 intrin.

`concat` uses C++11's variadic template to detect string's length and pre-alloc memory. I am not sure if recursive func calling would be inlined, so it may in fact hurt performance. Anyway, it's just a toy.

## License

common (including its component) is licensed under the [MIT license](../License.txt).