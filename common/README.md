# common

A collection of useful utilities

**Some components (\*.hpp) are header-only, while some (\*.h&\*.inl) are not. (\*.h) provides declaration and (\*.inl) provides implements.**

## Main Componennts

### [CommonRely](./CommonRely.hpp)

Basic header for most of utilities here, provides compiler check and some basic utilities like `hash_string`, `MACRO OPs`, `NonCopyable/NonMovable` 

### [EnumEx](./EnumEx.hpp)

Enum enhancement.

* **`BitField`** : `MAKE_ENUM_BITFIELD(x)`
  
  `enum class` with `BitFiled` mode can be easily performed bitwise operation. `HAS_FIELD` and `REMOVE_MASK` also provide simpler usage.

* **`Range`** : `MAKE_ENUM_RANGE(x)`
  
  `enum class` with `Range` mode can be easily performed comparasion and increasment/decreasement.

It also has experiemental compile-time reflection support based on `FUNCSIG` macro. The idea comes from [magic_enum](https://github.com/Neargye/magic_enum).

### [Exception](./Exceptions.hpp)

Custom Exception model, inherit from std::runtime_error, with support of nested-exception, strong-type, Unicode message, arbitrary extra data...

### [Linq](./Linq.hpp)

C#-like Linq implementation. It is compile-time based.

### [Stream](./Stream.hpp)

Stream is an abstract resouce that can read data from or write data to. 

* [Stream.hpp](./Stream.hpp) provides basic stream concept and buffered wrapper.

* [MemoryStream.hpp](./MemoryStream.hpp) provides stream from memory (and specially from contiguous container).

* [FileEx.hpp](./FileEx.hpp) provide stream for files. FileStream are acquired from `FileObject` which uses RAII to wrap file handle.

### [SpinLocker](./SpinLocker.hpp)

Spin-lock implemented using std::atomic. 

It also provided a read-write lock(both priority supported) and a prefer-lock, both are spin-locked.

### [TimeUtil](./TimeUtil.hpp)

A utility to provide time query support, mainly used as a timer.

### [SIMD](./SIMD.hpp) **`unfinished`**

It provides SSE/AVX static version test (compiler support).

[SIMD128](./SIMD128.hpp) and [SIMD256](./SIMD256.hpp) provide SIMD vector types support.

### [Wrapper](./Wrapper.hpp)

Wrapper can be simply regarded as a combination of `shared_ptr` and `make_shared`. It is mostly my own taste and may not be recommended.

Inside Wrapper I used some SFINAE tech to detect type, which may result in some compiler error --- SFINAE need class type fully defined, so you just can't use Wrapper for an incomplete type, which is common in forward-declaration.

### [SharedString](./SharedString.hpp)

A shared immutable string.
  
Some string is used across threads but not being modified, so they can be shared with proper lifetime management.

ShareString provides access using string_view and uses embedded reference-block to save pointer's dereference overhead.

### [PromiseTask](./PromiseTask.hpp)

`PromiseResut` is the foundation of other async operation utilities (like `AsyncExecutor`, `OpenGLUtil`, `OpenCLUtil`), which provides a common interface to operate a result which may not be ready yet.

Non-templated `PromiseResultCore` provides state and time infomation, which ease the promise storing with type-erasure. Because of this, `PromiseResut` should be stored in heap and wrapped by `shared_ptr`

`IsPromiseResult<T>()` and `EnsurePromiseResult<T>()` can be used to check if `T` is type of `PromiseResult`. `PromiseChecker` provides ability to return result type.

[PromiseTaskSTD.hpp](PromiseTaskSTD.hpp) is a wrapper for C++11's `future` and `promise`. It is seperated due to the incompatiblility with C++/CLI.

### [EasierJSON](./EasierJSON.hpp)

A wrapper for `rapidjson`, providing strong-typed json components and easier conversions and operations.

It uses shared_ptr to implicitly manage memory resources, which will lead to more overhead than original `rapidjson`.

* **`DocumentHandle`** wraps a MemoryPoolAllocator, can be use to create new `JArray` and `JObject`.

* **`JNode`** an abstract concept of JSON node (array or object), provides functionality for `Stringify()`. Any actual node type should use CRTP to inherit from it.

* **`JDoc`** a JSON document, which is basicly a `DocumentHandle` with `JNode`'s `Stringify` support. It can only be created from `Parse()` of string.

* **`JDocRef<B>`** wraps a `JDoc`, can be const reference.

* **`JComplexType`** an abstract concept of JSON node, provides functionality for getting inner data. Any actual node type should use CRTP to inherit from it.

* **`JArrayLike`** an abstract concept of JSON array, it uses `rapidjson::SizeType` as KeyType of `JComplexType`.

* **`JObjectLike`** an abstract concept of JSON object, it uses `common::u8StrView` as KeyType of `JComplexType`.

* **`JArray`** actual JSON array, uses `Push` to add data.

* **`JArrayRef<B>`** wraps a `JArray`, can be const reference.

* **`JObject`** actual JSON array, uses `Add` to add data.

* **`JObjectRef<B>`** wraps a `JObject`, can be const reference.

### [StringEx](./StringEx.hpp)

Some useful utilities for string operations like split, concat...

`split` is simply based on brute find, and there's no optimized implements like KMP or SSE4.2 intrin.

`concat` uses C++11's variadic template to detect string's length and pre-alloc memory. I am not sure if recursive func calling would be inlined, so it may in fact hurt performance. Anyway, it's just a toy.

### [StrCharset](./StrCharset.hpp)

StrCharset provide encoding defines and charset transform with self-made conversion class. Conversion is partial optimized, and partial-checked.

Converting encoding need to specify input charset, while StrCharset does not provide encoding-detection. If you need it , you should include [uchardet](../3rdParty/uchardetlib).

Thanks to `伐木丁丁鸟鸣嘤嘤`'s [GB18030-Unicode LUT](http://www.fmddlmyy.cn/text30.html), which is based on `谢振斌`'s work. [LUT_gb18030.tab](./LUT_gb18030.tab) is based on their works.

### [Controllable](./Controllable.hpp)

A base class using type erasure to support dynamic property access. object's property need to be registered explicitly.

**Controllable stores data inside the instance, so multi-inherited class may consider using virtual inheritance.**

**Controllable will soon be moved into a standalone dynamic library project to provide better metadata management**

### [SharedResource](./SharedResource.hpp)

An auto management for shared resource.
  
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

## Container Components

### [AlignedContainer](./AlignedContainer.hpp)

* **`AlignBase<size_t>`** custom base that support aligned memory allocation when created on heap

* **`AlignAllocator`** custom allocator that support aligned memory management

std containers with `AlignAllocator` is defined under `common::container` as xxx**Ex**.

### [AlignedBuffer](./AlignedBuffer.hpp)

A non-typed aligned memory space, provide sub-buffer creation and mamnagement(using reference counter).

### [ContainerHelper](./ContainerHelper.hpp)

Provides [contiguous container](https://en.cppreference.com/w/cpp/named_req/ContiguousContainer) type check.

### [ContainerEx](./ContainerEx.hpp)

A simple library providing some shortcuts for container operations, like comparator for tupel/pair, finding element in set/map/vector, creating a iteration-view for map's key/value.

* **`FrozenDenseSet`** an immutable set, which is in fact a sorted vector.

### [IntrusiveDoubleLinkList](./IntrusiveDoubleLinkList.hpp)

Bi-direction linked list with embedded node metadata.

NodeType should use CRTP to inherit from `IntrusiveDoubleLinkListNodeBase` and it will be `NonCopyable`.

`IntrusiveDoubleLinkList` uses [WRSpinLock](./SpinLock.hpp) as lock to provide thread-safty. However, **`AppendNode` only requires ReadScope, and they in fact uses CAS to achieve lock-free.** WriteScope is used when requires double-CAS and for blocking operations like `Clear` and `ForEach`.

## System Components

### [ResourceHelper](./ResourceHelper.h)

A wrapper to manage embedded resource. It uses DLL's embedded resource on Win32 and `binary object` on *nix.

On windows, you need to manually initalize the helper in dllmain. On *nix, initilize is done be global static variable initilization.

### [DelayLoader](./DelayLoader.h)

A wrapper to manage win32 DLL's delay-load feature.

### [FileMapper](./FileMapper.h) **`buggy`** **`unfinished`**

A wrapper to manage win32's file mapper

## License

common (including its component) is licensed under the [MIT license](../License.txt).