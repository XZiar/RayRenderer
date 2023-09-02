# StringUtil

## [Encoding Detection](StringDetect.h)

StringUtil uses [uchardet](../3rdParty/uchardet) to provide charset detection.

## [StrEncoding](StrEncoding.hpp)

StrEncoding provide encoding defines and charset transform with self-made conversion class. Conversion is partial optimized, and partial-checked.

Thanks to `伐木丁丁鸟鸣嘤嘤`'s [GB18030-Unicode LUT](http://www.fmddlmyy.cn/text30.html), which is based on `谢振斌`'s work. [LUT_gb18030.tab](LUT_gb18030.tab) is based on their works.

## [Encoding Conversion](StringConvert.h)

StringUtil wraps [StrEncoding](StrEncoding.hpp) to provide charset conversion, reducing the overhead and size by dynamic linking.

Since `codecvt` is marked deprecated in C++17 and some conversion seems to be locale-dependent.

## [Formating](Format.h)

StringUtil implements bytecode-based formating support, which can do **compile-time type check and syntax parsing**.

Format assumes UTF8 for `char` type, but also support `char8_t` when avaliable. Conversion between UTFs are provided by [Encoding Conversion](#encoding-conversion).

Format uses [fmt](../3rdParty/fmt) as backend to provide fast styled arg formating. 

### Restriction & Difference

Several restriction are applied:
* **Max length of formatter to be 65530**: due to bytecode design
* **Max indexed arg to be 64**: save parsing overhead due to the storage of arg info
* **Max named arg to be 16**: save parsing overhead of arg lookup

Some difference comapre to fmt:
* **Auto index and explicit index are supported the same time**: auto index follow the max index last being used.
* **Named arg cannot be indexed**: due to the need of compile-time self type-check. fmt is avoiding it by disabling switch between auto/explicit index.
* **char display of bool to be Y/N**: in fmt and python, it's '\0x1' or '0x0', which seem not readable.
* **support mixing char type**


### Design

Following explian the usually steps how formating happens:
#### formatter to bytecode
Formatetr string is being parsed into bytecode and metadata. Metadata includes display type request to be checked with actual arg.

#### arg type packing
Arg type is being packed into ArgInfo, which can be done at compile time so that it can be checked with formatter.

#### type-checking
Formatter's type request and real arg type is being checked, also named arg is being checked and return the mapping between named arg and index.

#### arg packing
Arg of formating is always being packed and copied, [boost's `small_vector`](https://www.boost.org/doc/libs/1_80_0/doc/html/boost/container/small_vector.html) is being used for optimization.

The minimal unit in the pack is 2byte, so some arg may not be fully aligned. **Unaligned read support is required**.

Pack stores slot index at the begining and arg data is followed.

#### executor
Formatter metadata, arg info, arg pack are stored in context. lib iterate the bytecodes and proxy the real action to executor with context.

Executor is typed-erased so that maximun flexibility can be provided. Arg locating are done by executor so it's possible to bypass arg-copy with custom executor.

StaticExecutor is provided and internally inlined to provide performance in usual cases.

#### formatter
Executor calls formatter to do actual formation, where formatter is usually just wrap of `fmt`.

Custom formatter can hook up format call to record actions or arg. E.g, with `LoggerFormatter`(MiniLogger.h), color info will be manually stored and then it can be used with any target that supports color-segement-based text.

## [fmt](StringFormt.h)

StringUtil also exposes the embedded [fmt](../3rdParty/fmt), with dynamic-linking to reduce some binary size overhead. 

# Dependency

* [uchardet](https://www.freedesktop.org/wiki/Software/uchardet/) `submodule` 0.0.8

  [MPL 1.1 License](./3rdParty/uchardet/COPYING)

* [fmt](http://fmtlib.net) `submodule` 10.1.0

  [MIT License](./3rdParty/fmt/LICENSE.rst)

# License

StringUtil (including its component) is licensed under the [MIT license](../License.txt).
