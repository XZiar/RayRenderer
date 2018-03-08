# RayRenderer

A messy renderer based on my old RayTrace project.

But it's not a renderer yet, it's just a collection of utilities.

### Changes from old project

The old preject is [here](https://github.com/XZiar/RayTrace)

* fixed pipeline --> GLSL shader
* x64/AVX2 only RayTracing --> multi versions of acceleration RayTracing(may also include OpenCL)
* GLUT based GUI --> simple GUI with FreeGLUT, complex GUI with WPF
* coupling code --> reusable components

## Componoent

| | |
|:-------|:-------:|
| [3rdParty](./3rdParty) | 3rd party library |
| [3DBasic](./3DBasic) | Self-made BLAS library and simple 3D things |
| [common](./common) | Basic but useful things |
| [CommonUtil](./CommonUtil) | Basic utils for C# |
| [miniLogger](./common/miniLogger) | Mini Logger |
| [AsyncExecutor](./common/AsyncExecutor) | Async Executor |
| [ImageUtil](./ImageUtil) | Image Read/Write Utility |
| [OpenGLUtil](./OpenGLUtil) | Wrapper of OpenGL things |
| [OpenCLUtil](./OpenCLUtil) | Wrapper of OpenCL things |
| [FontHelper](./FontHelper) | Helper for displaying font in OpenGL |
| [FreeGLUTView](./FreeGLUTView) | Wrapper of FreeGLUT |
| [OpenGLView](./OpenGLView) | Wrapper of OpenGL window in WinForm |
| [RenderCore](./RenderCore) | Core of RayRenderer |
| [RenderCoreWrap](./RenderCoreWrap) | C++/CLI Wrapper for RayRender core |
| [GLUTTest](./GLUTTest) | Test Program(C++) (using FreeGLUTView) |
| [WinFormTest](./WinFormTest) | Test Program(C#) in WinForm (using OpenGLView) |
| [WPFTest](./WPFTest) | Test Program(C#) in WPF (using OpenGLView) |

## Platform

Since C++/CLI is used for C# bindings, and multiple DLL hacks are token for DLL-embedding, it's Windows-only.

VS2017(`15.5.3`) needed. Windows SDK Target is `10.0.16299.0`. .Net Framework 4.7.1 needed for C# components.

## Additional Requirements

Some VC++ default props should be set --- `include path` and `libpath`.

`boost` headers folder should be found inside `include path`.

`gl*.h` headers should be found inside `include path\GL`.

[`opencl*.h` headers](https://github.com/KhronosGroup/OpenCL-Headers) should be found inside `include path\CL`.

## Dependency

* [GLEW](http://glew.sourceforge.net/)  2.1.0

  [Modified BSD & MIT License](./License/glew.txt)

* [FreeGLUT](http://freeglut.sourceforge.net)  3.0.0

  [MIT License](./License/freeglut.txt)

* [boost](http://www.boost.org/)  1.66.0 (not included in this repo)

  [Boost Software License](./License/boost.txt)

* [stb](https://github.com/nothings/stb)

  [MIT License](./License/stb.txt)

* [fmt](http://fmtlib.net) 4.1.0 (customized with utf-support)

  [BSD-2 License](./License/fmt.rst)

* [uchardet](https://www.freedesktop.org/wiki/Software/uchardet/) 0.0.6

  [MPL 1.1 License](./License/uchardet.txt)

* [FreeType2](https://www.freetype.org/) 2.8.1

  [The FreeType License](./License/freetype.txt)

* [cpplinq](http://cpplinq.codeplex.com/)

  [MS-PL License](./License/cpplinq.html)

* [date](https://howardhinnant.github.io/date/date.html) 2.4

  [MIT License](./License/date.txt)

* [OpenCL-ICD-Loader](https://github.com/KhronosGroup/OpenCL-ICD-Loader)

  [License](./3rdParty/OpenCL_ICD_Loader/LICENSE.txt)

## License

RayRenderer (including its component) is licensed under the [MIT license](License.txt).
