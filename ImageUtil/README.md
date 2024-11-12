# ImageUtil

A simple utility for image read/write/process.

Image's datatype convertion is SIMD optimized.

## Endianness

Internal data layout is assumed to be Little-endian.

## Format Support

### Zex

| Type | Support Format | Access |
|:-------|:-------:|:------:|
| TGA | RGB/RGBA/Gray/RLE | R/W |
| BMP | RGB/RGBA/Gray | R/W |

### Stb
| Type | Support Format | Access |
|:-------|:-------:|:------:|
| PNG | RGB/RGBA/Gray | R/W |
| TGA | RGB/RGBA/Gray | R/W |
| JPEG | RGB/Gray | R/W |
| BMP | RGB/RGBA/Gray | R/W |
| PNM | RGB | R |

### libpng
| Type | Support Format | Access |
|:-------|:-------:|:------:|
| PNG | RGB/RGBA/Gray | R/W |

### libjpeg-turbo
| Type | Support Format | Access |
|:-------|:-------:|:------:|
| JPEG | RGB | R/W |

### Wic
Windows Image Codec
| Type | Support Format | Access |
|:-------|:-------:|:------:|
| JPEG | RGB/RGBA/Gray | R/W |
| BMP | RGB/RGBA/Gray | R/W |
| PNG | RGB/RGBA/Gray | R/W |
| HEIF | RGB/RGBA/Gray | R/W |
| WEBP | RGB/RGBA/Gray | R/W |

### Ndk
Android Image Decoder
| Type | Support Format | Access |
|:-------|:-------:|:------:|
| JPEG | RGBA/Gray | R/W |
| BMP | RGBA/Gray | R |
| PNG | RGBA/Gray | R/W |
| HEIF | RGBA/Gray | R |
| WEBP | RGBA/Gray | R/W |

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

* [zlib-ng](https://github.com/zlib-ng/zlib-ng) `submodule` 2.2.2

  [zlib License](../3rdParty/zlib-ng/LICENSE.md)

* [libpng](http://www.libpng.org/pub/png/libpng.html) `submodule` 1.6.44

  [libpng License](../3rdParty/libpng/LICENSE)

* [libjpeg-turbo](https://www.libjpeg-turbo.org/Main/HomePage) `submodule`  3.0.4

  [IJG License, BSD-3 License, zlib License](../3rdParty/libjpeg-turbo/LICENSE.md)
  
* [stb](https://github.com/nothings/stb) `submodule`

  [MIT License](../3rdParty/stb/LICENSE)

## License

ImageUtil (including its component) is licensed under the [MIT license](License.txt).

This software is based in part on the work of the Independent JPEG Group.