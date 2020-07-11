## StringUtil

### [Charset Detection](Detect.h)

StringUtil uses [uchardet](../3rdParty/uchardetlib) to privede charset detection.

### [Charset Conversion](Convert.h)

StringUtil wraps [StrCharset](../common/StrCharset.hpp) to provide charset conversion, reducing the overhead and size by dynamic linking.

Since `codecvt` is marked deprecated in C++17 and some conversion seems to be locale-dependent.

### [String Formating](Format.h)

StringUtil wraps [fmt](../3rdPart/fmt) to provide string formation, with utf extension supported. 

Some proxy for utf string conversion was made inside fmt's source code, while majority of the extension code is inside StringUtil.

## Dependency

* [uchardet](https://www.freedesktop.org/wiki/Software/uchardet/) 0.0.6 @2018-09-26

  [MPL 1.1 License](./3rdParty/uchardetlib/license.txt)

* [fmt](http://fmtlib.net) 7.0.1 (customized with utf-support)

  [MIT License](./3rdParty/fmt/LICENSE.rst)

## License

StringUtil (including its component) is licensed under the [MIT license](../License.txt).
