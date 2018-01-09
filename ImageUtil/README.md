# ImageUtil

A simple utility for image read/write/process.

Image converting is AVX/SSE optimized.

## Endianness

Since This component is binded with X86 optimization, internal data layout is assumed to be Little-endian.

## Current Support

| | |
|:-------|:-------:|
| Type | Support | Source |
| PNG | RGB/RGBA/Gray | libpng |
| TGA | RGB/RGBA/Gray | zextga(self) |
| JPEG | RGBA | libjpeg-turbo |
| BMP | RGB/RGBA | zexbmp(self) |


## Dependency

* [zlib](http://www.zlib.net/zlib.html)  1.2.11

  [zlib License](./License/zlib.txt)

* [libpng](http://www.libpng.org/pub/png/libpng.html)  1.6.34

  [libpng License](./License/libpng.txt)

* [libjpeg-turbo](http://www.libjpeg-turbo.org/Main/HomePage)  1.5.3

  [IJG License, BSD-3 License, zlib License](./License/libjpeg-turbo.txt)

## License

ImageUtil (including its component) is licensed under the [MIT license](License.txt).

This software is based in part on the work of the Independent JPEG Group.