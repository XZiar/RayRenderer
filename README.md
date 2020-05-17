# RayRenderer

[![Build Status](https://travis-ci.org/XZiar/RayRenderer.svg?branch=master)](https://travis-ci.org/XZiar/RayRenderer)

No it's not a renderer.

That's just an excuse for me to write a collection of utilities which "will be used for the renderer".

Maintaing and growing the project is much more fun than make it actually renderable. :sweat_smile:

### Changes from old project

The old preject is [here](https://github.com/XZiar/RayTrace)

* fixed pipeline --> GLSL shader
* x64/AVX2 only RayTracing --> multi versions of acceleration RayTracing(may also include OpenCL)
* GLUT based GUI --> simple GUI with FreeGLUT, complex GUI with WPF
* coupling code --> reusable components

## Component

| Component | Description | Language | Platform |
|:-------|:-------:|:---:|:-------:|
| [3rdParty](./3rdParty) | 3rd party library | C,C++ | N/A |
| [common](./common) | Basic but useful things | Multi | N/A |
| [3DBasic](./3DBasic) | Self-made BLAS library and simple 3D things | C++ | N/A |
| [StringCharset](./StringCharset) | String Charset Library | C++ | Windows & Linux |
| [SystemCommon](./SystemCommon) | System-level common library | C++ | Windows & Linux |
| [MiniLogger](./MiniLogger) | Mini Logger | C++ | Windows & Linux |
| [Nailang](./Nailang) | A customizable language | C++ | Windows & Linux |
| [AsyncExecutor](./AsyncExecutor) | Async Executor | C++ | Windows & Linux |
| [ImageUtil](./ImageUtil) | Image Read/Write Utility | C++ | Windows & Linux |
| [OpenGLUtil](./OpenGLUtil) | Wrapper of OpenGL things | C++ | Windows & Linux |
| [OpenCLUtil](./OpenCLUtil) | Wrapper of OpenCL things | C++ | Windows & Linux |
| [OpenCLInterop](./OpenCLInterop) | OpenCL Interoperation utility | C++ | Windows & Linux |
| [FontHelper](./FontHelper) | Helper for displaying font in OpenGL | C++ | Windows & Linux |
| [TextureUtil](./TextureUtil) | Texture Utility | C++ | Windows & Linux |
| [WindowHost](./WindowHost) | Multi-threaded GUI host | C++ | Windows & Linux |
| [FreeGLUTView](./FreeGLUTView) | Wrapper of FreeGLUT | C++ | Windows & Linux |
| [ResourcePackager](./ResourcePackager) | Resource (de)serialize support | C++ | Windows & Linux |
| [RenderCore](./RenderCore) | Core of RayRenderer | C++ | Windows & Linux |
| [RenderCoreWrap](./RenderCoreWrap) | C++/CLI Wrapper for RayRender core | C++/CLI | Windows |
| [CommonUtil](./CommonUtil) | Basic utilities for C# | C# | Windows |
| [AnyDock](./AnyDock) | Flexible dock layout like AvalonDock for WPF | C# | Windows |
| [OpenGLView](./OpenGLView) | Wrapper of OpenGL window in WinForm | C++/CLI | Windows |
| [GLUTTest](./GLUTTest) | Test Program(C++) (using FreeGLUTView) | C++ | Windows & Linux |
| [UtilTest](./Tests/UtilTest) | Utilities Test Program(C++) | C++ | Windows & Linux |
| [WPFTest](./WPFTest) | Test Program(C#) in WPF (using OpenGLView) | C#  | Windows |

## Platform Requirements

Since C++/CLI is used for C# bindings, and multiple DLL hacks are token for DLL-embedding, it's Windows-only.

For Windows parts, Windows SDK Target is `10(latest)` and .Net Framework 4.8 needed for C# components.

To use `xzbuild`, python3.6+ is required.

## Build

To build C++ parts, an C++17 compiler is needed. For Linux, GCC7.3 and later are tested. For Windows, VS2019(`16.0`) is needed for the vcproj version.

Project uses `VisualStudio2019` on Windows, and uses `xzbuild` with make on Linux. Utilities that have `xzbuild.proj.json` inside are capable to be compiled on Linux, tested on GCC(7.3&8.0) and Clang(7.0&8.0).

They can be built by execute [`xzbuild.py`](xzbuild.py) (python3.6+).

### Additional Requirements

`boost` headers folder should be found inside `include path`. It can be added in [SolutionInclude.props](./SolutionInclude.props) in Windows, or specified by environment variable `CPP_DEPENDENCY_PATH`.

`gl.h` and `glu.h` headers should be found inside `include path\GL`.

[nasm](https://www.nasm.us/) needed for [libjpeg-turbo](./3rdParty/libjpeg-turbo) --- add it to system environment path

[ispc compiler](https://ispc.github.io/downloads.html) needed for [ispc_texcomp](./3rdParty/ispc_texcomp) --- add it to system environment path

## Dependency

* [gsl](https://github.com/microsoft/GSL) 3.0.1
  
  [MIT License](./3rdParty/gsl/LICENSE)

* [FreeGLUT](http://freeglut.sourceforge.net)  3.2.1

  [MIT License](./3rdParty/freeglut/license.txt)

* [boost](http://www.boost.org/)  1.73.0 (not included in this repo)

  [Boost Software License](./License/boost.txt)

* [fmt](http://fmtlib.net) 6.2.1 (customized with utf-support)

  [MIT License](./3rdParty/fmt/LICENSE.rst)

* [rapidjson](http://rapidjson.org/) 1.1.0 master@2020-04-10

  [MIT License](./3rdParty/rapidjson/license.txt)

* [digestpp](https://github.com/kerukuro/digestpp) master@2020-02-04
  
  [Public Domain](3rdParty/digestpp/LICENSE)

* [FreeType2](https://www.freetype.org/) 2.10.1

  [The FreeType License](./3rdParty/freetype2/license.txt)

* [ISPCTextureCompressor](https://github.com/GameTechDev/ISPCTextureCompressor)
  
  [MIT License](./3rdParty/ispc_texcomp/license.txt)

* [itoa](https://github.com/miloyip/itoa-benchmark)

* [half](http://half.sourceforge.net/) 2.1.0
  
  [MIT License](./3rdParty/half/LICENSE.txt)

* [libcpuid](http://libcpuid.sourceforge.net/) 0.4.1 master@2020-01-02

  [BSD License](./3rdParty/cpuid/COPYING)

## License

RayRenderer (including its component) is licensed under the [MIT license](License.txt).
