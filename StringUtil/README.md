## StringUtil

### [Charset Detection](Detect.h)

StringUtil uses [uchardet](../3rdParty/uchardet) to provide charset detection.

### [StrCharset](StrCharset.hpp)

StrCharset provide encoding defines and charset transform with self-made conversion class. Conversion is partial optimized, and partial-checked.

Thanks to `伐木丁丁鸟鸣嘤嘤`'s [GB18030-Unicode LUT](http://www.fmddlmyy.cn/text30.html), which is based on `谢振斌`'s work. [LUT_gb18030.tab](LUT_gb18030.tab) is based on their works.

### [Charset Conversion](Convert.h)

StringUtil wraps [StrCharset](StrCharset.hpp) to provide charset conversion, reducing the overhead and size by dynamic linking.

Since `codecvt` is marked deprecated in C++17 and some conversion seems to be locale-dependent.

### [String Formating](Format.h)

StringUtil wraps [fmt](../3rdPart/fmt) to provide string formation, with utf extension supported. 

Some proxy for utf string conversion was made inside fmt's source code, while majority of the extension code is inside StringUtil.

## Dependency

* [uchardet](https://www.freedesktop.org/wiki/Software/uchardet/) `submodule` 0.0.7

  [MPL 1.1 License](./3rdParty/uchardet/COPYING)

* [fmt](http://fmtlib.net) `submodule` 7.1.2 (customized with utf-support)

  [MIT License](./3rdParty/fmt/LICENSE.rst)

## License

StringUtil (including its component) is licensed under the [MIT license](../License.txt).
