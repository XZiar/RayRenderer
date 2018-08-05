# RayRenderer

[![Build Status](https://travis-ci.org/XZiar/RayRenderer.svg?branch=master)](https://travis-ci.org/XZiar/RayRenderer)

A messy renderer based on my old RayTrace project.

But it's not a renderer yet, it's just a collection of utilities.

### Changes from old project

The old preject is [here](https://github.com/XZiar/RayTrace)

* fixed pipeline --> GLSL shader
* x64/AVX2 only RayTracing --> multi versions of acceleration RayTracing(may also include OpenCL)
* GLUT based GUI --> simple GUI with FreeGLUT, complex GUI with WPF
* coupling code --> reusable components

## Component

| Component | Description | Platform |
|:-------|:-------:|:-------:|
| [3rdParty](./3rdParty) | 3rd party library | N/A |
| [3DBasic](./3DBasic) | Self-made BLAS library and simple 3D things | Windows & Linux |
| [common](./common) | Basic but useful things | N/A |
| [CommonUtil](./CommonUtil) | Basic utilities for C# | Windows |
| [miniLogger](./common/miniLogger) | Mini Logger | Windows & Linux |
| [AsyncExecutor](./common/AsyncExecutor) | Async Executor | Windows & Linux |
| [ImageUtil](./ImageUtil) | Image Read/Write Utility | Windows & Linux |
| [OpenGLUtil](./OpenGLUtil) | Wrapper of OpenGL things | Windows |
| [OpenCLUtil](./OpenCLUtil) | Wrapper of OpenCL things | Windows |
| [FontHelper](./FontHelper) | Helper for displaying font in OpenGL | Windows |
| [TextureUtil](./TextureUtil) | Texture Utility | Windows |
| [ResourcePackager](./ResourcePackager) | Resource package and management library | Windows & Linux|
| [FreeGLUTView](./FreeGLUTView) | Wrapper of FreeGLUT | Windows |
| [OpenGLView](./OpenGLView) | Wrapper of OpenGL window in WinForm | Windows |
| [RenderCore](./RenderCore) | Core of RayRenderer | Windows |
| [RenderCoreWrap](./RenderCoreWrap) | C++/CLI Wrapper for RayRender core | Windows |
| [GLUTTest](./GLUTTest) | Test Program(C++) (using FreeGLUTView) | Windows |
| [WinFormTest](./WinFormTest) | Test Program(C#) in WinForm (using OpenGLView) | Windows |
| [WPFTest](./WPFTest) | Test Program(C#) in WPF (using OpenGLView) | Windows |

## Platform Requirements

Since C++/CLI is used for C# bindings, and multiple DLL hacks are token for DLL-embedding, it's Windows-only.

VS2017(`15.7`) needed. Windows SDK Target is `10.0.17134.0`. .Net Framework 4.7.1 needed for C# components.

Some Utilities have `Makefile` inside are capable to be compiled on Linux, only tested on GCC 7.2. They can be built by execute [`build.sh`](build.sh) or use more powerful build system via [`build.py`](build.py) (python3).

`ResoucePackager` cannot be compiled by GCC7.3/7.3.1 due to compiler bug with rapidjson, see Tencent/rapidjson#1338

Several C++17 technic are taken, like STL-Containers(`string_view`, `any`, `optional`, `variant`), `constexpr-if`, structureed-binding, selection-statements-initializers, inline variables.

## Additional Requirements

Some VC++ default props should be set --- `include path` and `libpath`.

`boost` headers folder should be found inside `include path`, or inside environment `CPP_DEPENDENCY_PATH`.

`gl*.h` headers should be found inside `include path\GL`.

[nasm](https://www.nasm.us/) needed for [libjpeg-turbo](./3rdParty/libjpeg-turbo) --- add it to system environment path

[ispc compiler](https://ispc.github.io/downloads.html) needed for [ispc_texcomp](./3rdParty/ispc_texcomp) --- add it to system environment path

## Dependency

* [GLEW](http://glew.sourceforge.net/)  2.1.0

  [Modified BSD & MIT License](./3rdParty/glew/license.txt)

* [FreeGLUT](http://freeglut.sourceforge.net)  3.0.0

  [MIT License](./3rdParty/freeglut/license.txt)

* [boost](http://www.boost.org/)  1.67.0 (not included in this repo)

  [Boost Software License](./License/boost.txt)

* [stb](https://github.com/nothings/stb)

  [MIT License](./3rdParty/stblib/license.txt)

* [fmt](http://fmtlib.net) 5.1.0 (customized with utf-support)

  [BSD-2 License](./3rdParty/fmt/license.rst)

* [crypto++](https://www.cryptopp.com) 7.0

  [Boost Software License](./3rdParty/cryptopp/license.txt)

* [rapidjson](http://rapidjson.org/) 1.1.0 master@2018-06-05

  [MIT License](./3rdParty/rapidjson/license.txt)

* [uchardet](https://www.freedesktop.org/wiki/Software/uchardet/) 0.0.6

  [MPL 1.1 License](./3rdParty/uchardetlib/license.txt)

* [FreeType2](https://www.freetype.org/) 2.8.1

  [The FreeType License](./3rdParty/freetype2/license.txt)

* [cpplinq](http://cpplinq.codeplex.com/)

  [MS-PL License](./3rdParty/cpplinq.html)

* [ISPCTextureCompressor](https://github.com/GameTechDev/ISPCTextureCompressor)
  
  [MIT License](./3rdParty/ispc_texcomp/license.txt)

* [date](https://howardhinnant.github.io/date/date.html) 2.4.1

  [MIT License](./3rdParty/date/LICENSE.txt)

* [OpenCL-ICD-Loader](https://github.com/KhronosGroup/OpenCL-ICD-Loader)

  [License](./3rdParty/OpenCL_ICD_Loader/LICENSE.txt)

* [itoa](https://github.com/miloyip/itoa-benchmark)

## License

RayRenderer (including its component) is licensed under the [MIT license](License.txt).
