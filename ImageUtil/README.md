# ImageUtil

A simple utility for image read/write/process.

Image's datatype convertion is AVX/SSE optimized.

## Endianness

Since This component is binded with X86 optimization, internal data layout is assumed to be Little-endian.

## Current Support

| Type | Support Format | Provider |
|:-------|:-------:|:------:|
| PNG | RGB/RGBA/Gray | libpng |
| TGA | RGB/RGBA/Gray | zextga(self) |
| JPEG | RGB/Gray | libjpeg-turbo |
| BMP | RGB/RGBA | zexbmp(self) |


## Dependency

* [zlib](http://www.zlib.net/zlib.html)  1.2.11

  [zlib License](../3rdParty/zlib/license.txt)

* [zlib-ng](https://github.com/Dead2/zlib-ng)  1.9.9 develop@2018-09-19

  [zlib License](../3rdParty/zlib-ng/LICENSE.md)

* [libpng](http://www.libpng.org/pub/png/libpng.html)  1.6.35

  [libpng License](../3rdParty/libpng/LICENSE)

* [libjpeg-turbo](http://www.libjpeg-turbo.org/Main/HomePage)  2.0.0

  [IJG License, BSD-3 License, zlib License](../3rdParty/libjpeg-turbo/LICENSE.md)

## License

ImageUtil (including its component) is licensed under the [MIT license](License.txt).

This software is based in part on the work of the Independent JPEG Group.