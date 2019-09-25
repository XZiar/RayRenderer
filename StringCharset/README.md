## StringCharset

StringCharset wraps [StrCharset](../common/StrCharset.hpp) to provide charset conversion, reducing the overhead and size by dynamic linking.

It also uses [uchardet](../3rdParty/uchardetlib) to privede charset detection, since `codecvt` is marked deprecated in C++17 and some conversion seems to be locale-dependent.

## License

StringCharset (including its component) is licensed under the [MIT license](../License.txt).
