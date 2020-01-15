# 3rdParty libraries

This folder includes almost all the 3rd-party libraries that RayRenderer uses.

It seems that the normal way is to fork each of them, and add them here by "submodule".
However, I am using Visual Studio whose github plugin has no such function for me.
Also, I am tired to find out the difference among "submodule", "subtree", etc...

## Dummy libraries

Some libraries are previously used but still remian in the repo:

* [GLEW](http://glew.sourceforge.net/)  2.1.0

  [Modified BSD & MIT License](./glew/license.txt)

* [date](https://howardhinnant.github.io/date/date.html) 2.4.1

  [MIT License](./date/LICENSE.txt)

## Libraries for test

Some libraries are only used for test app:

* [libressl](http://www.libressl.org/) 2.9.0
  
  [Apache License](./3rdParty/libressl/COPYING)

* [curl](https://curl.haxx.se/libcurl/) 7.63.0
  
  [Modified MIT License](./3rdParty/curl/LICENSE-MIXING.md)

## License

Each library is licensed under its license. You can find a copy under its library.