# ImageUtil

A simple utility for image read/write/process.

Image's datatype convertion is AVX/SSE optimized.

## Endianness

Since This component is binded with X86 optimization, internal data layout is assumed to be Little-endian.

## Format Support

| Type | Support Format | Provider |
|:-------|:-------:|:------:|
| PNG | RGB/RGBA/Gray | libpng / stb |
| TGA | RGB/RGBA/Gray | zextga(self) / stb |
| JPEG | RGB/Gray | libjpeg-turbo / stb |
| BMP | RGB/RGBA | zexbmp(self) / stb |
| PNM | RGB | stb |

## [TextureFormat](./TexFormat.h)

`TextureFormat` provides an universal representation for texture data format, which mainly focus on GPU-related texture type.

It's bitfield arrangement is listed below
```
[unused|sRGB|channel-order|channels|type categroy|data type]
[------|*14*|13.........12|11.....8|7...........6|5.......0]
```
* Data Type Category
  * **Plain** Normal type
  * **Composite** Composite type, combine multiple channel into bitfields
  * **Compress** Compressed type

`ImdageDataFormat` is just a small subset of it and is used to simplify format check/conversion.

## Dependency

* [zlib-ng](https://github.com/zlib-ng/zlib-ng) `submodule` 2.0.6

  [zlib License](../3rdParty/zlib-ng/LICENSE.md)

* [libpng](http://www.libpng.org/pub/png/libpng.html) `submodule` 1.6.39

  [libpng License](../3rdParty/libpng/LICENSE)

* [libjpeg-turbo](https://www.libjpeg-turbo.org/Main/HomePage) `submodule`  2.1.90

  [IJG License, BSD-3 License, zlib License](../3rdParty/libjpeg-turbo/LICENSE.md)
  
* [stb](https://github.com/nothings/stb) `submodule`

  [MIT License](../3rdParty/stb/LICENSE)

## License

ImageUtil (including its component) is licensed under the [MIT license](License.txt).

This software is based in part on the work of the Independent JPEG Group.