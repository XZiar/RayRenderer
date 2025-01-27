# RayRenderer

[![CI](https://github.com/XZiar/RayRenderer/workflows/CI/badge.svg)](https://github.com/XZiar/RayRenderer/actions)

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
| [SystemCommon](./SystemCommon) | System-level common library | C++ | Win & Linux & Android & iOS |
| [Nailang](./Nailang) | A customizable language | C++ | Win & Linux & Android & iOS |
| [ImageUtil](./ImageUtil) | Image Read/Write Utility | C++ | Win & Linux & Android & iOS |
| [XComputeBase](./XComputeBase) | Base Library for Cross-Compute | C++ | Win & Linux & Android & iOS |
| [OpenGLUtil](./OpenGLUtil) | Wrapper of OpenGL things | C++ | Win & Linux |
| [OpenCLUtil](./OpenCLUtil) | Wrapper of OpenCL things | C++ | Win & Linux & Android |
| [OpenCLInterop](./OpenCLInterop) | OpenCL Interoperation utility | C++ | Win & Linux |
| [FontHelper](./FontHelper) | Helper for displaying font in OpenGL | C++ | Win & Linux |
| [TextureUtil](./TextureUtil) | Texture Utility | C++ | Win & Linux |
| [WindowHost](./WindowHost) | Multi-threaded GUI host | C++ | Win & Linux |
| [ResourcePackager](./ResourcePackager) | Resource (de)serialize support | C++ | Win & Linux |
| [RenderCore](./RenderCore) | Core of DizzRenderer | C++ | Win & Linux |
| [RenderCoreWrap](./RenderCoreWrap) | C++/CLI Wrapper for RenderCore | C++/CLI | Win |
| [CommonUtil](./CommonUtil) | Basic utilities for C# | C# | Win |
| [AnyDock](./AnyDock) | Flexible dock layout like AvalonDock for WPF | C# | Win |
| [OpenGLView](./OpenGLView) | Wrapper of OpenGL window in WinForm | C++/CLI | Win |
| [DizzTest](./Tests/DizzTest) | Test Program(C++) (using WindowHost) | C++ | Win & Linux |
| [UtilTest](./Tests/UtilTest) | Utilities Test Program(C++) | C++ | Win & Linux |
| [WPFTest](./WPFTest) | Test Program(C#) in WPF (using OpenGLView) | C# | Win |

## Platform Requirements

Since C++/CLI is used for C# bindings, and multiple DLL hacks are token for DLL-embedding, it's Windows-only.

For Windows parts, Windows SDK Target is `10(latest)`. `.Net Core 8.0` needed for C# components.

To use `xzbuild`, python3.7+ is required.

## Build

To build C++ parts, a C++20 compiler is needed. CI Tests are on gcc(10\~14) and clang(10\~17).

For Windows, project uses `VisualStudio2022`, VS2022(>`17.6`) is needed for the vcproj version.

For Linux, project uses [`xzbuild`](./xzbuild) (need python3.7+) with make. Utilities that have `xzbuild.proj.json` inside are capable to be compiled on Linux.

### Incompatibles
* **gcc9** does not support `constexpr std::array::operator==`
* **clang13**/**clang14** has compatibility issue[1](https://github.com/llvm/llvm-project/issues/55560)/[2](https://reviews.llvm.org/D128119) with libstdc++


> Example usage of [`xzbuild`](./xzbuild)

```shell
python3 xzbuild help
python3 xzbuild list
python3 xzbuild build all
python3 xzbuild buildall UtilTest
```

### Additional Requirements

`boost` headers folder should be found inside `include path`. It can be added in [SolutionInclude.props](./SolutionInclude.props) on Windows, or specified by environment variable `CPP_DEPENDENCY_PATH` on *nix.

`gl.h` and `glu.h` headers should be found inside `include path\GL`.

[nasm](https://www.nasm.us/) needed for [libjpeg-turbo](./3rdParty/libjpeg-turbo) --- add it to system environment path

[ispc compiler](https://ispc.github.io/downloads.html) needed for [ispc_texcomp](./3rdParty/ISPCTextureCompressor) --- add it to system environment path.

## Dependency

* [gsl](https://github.com/microsoft/GSL) `submodule` 4.1.0
  
  [MIT License](https://github.com/microsoft/GSL/blob/main/LICENSE)

* [Google Test](https://github.com/google/googletest) `submodule` 1.15.2

  [BSD 3-Clause License](https://github.com/google/googletest/blob/master/LICENSE)

* [boost](http://www.boost.org/)  1.87.0 (not included in this repo)

  [Boost Software License](./License/boost.txt)

* [fmt](https://fmt.dev/) `submodule` 11.1.3

  [MIT License](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)

* [ghc-filesystem](https://github.com/gulrak/filesystem) `submodule` 1.5.14

  [MIT License](https://github.com/gulrak/filesystem/blob/master/LICENSE)

* [rapidjson](http://rapidjson.org/) `submodule` 1.1.0 master

  [MIT License](https://github.com/Tencent/rapidjson/blob/master/license.txt)

* [digestpp](https://github.com/kerukuro/digestpp) `submodule` master
  
  [Public Domain](https://github.com/kerukuro/digestpp/blob/master/LICENSE)

* [FreeType](https://www.freetype.org/) `submodule` 2.13.2

  [The FreeType License](https://github.com/freetype/freetype/blob/master/LICENSE.TXT)

* [ISPCTextureCompressor](https://github.com/GameTechDev/ISPCTextureCompressor) `submodule`
  
  [MIT License](https://github.com/GameTechDev/ISPCTextureCompressor/blob/master/license.txt)

* [half](http://half.sourceforge.net/) 2.2.0
  
  [MIT License](./3rdParty/half/LICENSE.txt)

* [cpuinfo](https://github.com/pytorch/cpuinfo) `submodule`

  [BSD 2-Clause  License](https://github.com/pytorch/cpuinfo/blob/main/LICENSE)

## License

RayRenderer (including its component) is licensed under the [MIT license](License.txt).
