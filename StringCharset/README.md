## StringCharset

StringCharset wraps [StrCharset](../common/StrCharset.hpp) to provide charset conversion, reducing the overhead and size by dynamic linking.

It also uses [uchardet](../3rdParty/uchardetlib) to privede charset detection, since `codecvt` is marked deprecated in C++17 and some conversion seems to be locale-dependent.

## Dependency

* [uchardet](https://www.freedesktop.org/wiki/Software/uchardet/) 0.0.6 @2018-09-26

  [MPL 1.1 License](./3rdParty/uchardetlib/license.txt)

## License

StringCharset (including its component) is licensed under the [MIT license](../License.txt).
