# StrCharset

StrCharset provide encoding defines and charset transform with self-made conversion class. Correctness check is not completed.

`codecvt` is removed since it is marked deprecated in C++17 and some conversion seems to be locale-dependent.

Converting encoding need to specify input charset, while StrCharset does not provide encoding-detection. If you need it , you should include [uchardet](../3rdParty/uchardetlib).

## License

AsyncExecutor (including its component) is licensed under the [MIT license](../License.txt).
