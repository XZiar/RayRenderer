# RayRenderer

A messy renderer based on my old RayTrace project.

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
| [miniLogger](./common/miniLogger) | mini logger |
| [ImageUtil](./ImageUtil) | Image Read/Write Utility |
| [OpenGLUtil](./OpenGLUtil) | Wrapper of OpenGL things |
| [OpenCLUtil](./OpenCLUtil) | Wrapper of OpenCL things |
| [FontHelper](./FontHelper) | Helper for displaying font in OpenGL |
| [FreeGLUTView](./FreeGLUTView) | Wrapper of FreeGLUT |
| [OpenGLView](./OpenGLView) | Wrapper of OpenGL window in WinForm |
| [RenderCore](./RenderCore) | Core of RayRender |
| [RenderCoreWrap](./RenderCoreWrap) | C++/CLI Wrapper for RayRender core |
| [GLUTTest](./GLUTTest) | Test Program(C++) (using FreeGLUTView) |
| [WinFormTest](./WinFormTest) | Test Program(C#) in WinForm (using OpenGLView) |
| [WPFTest](./WPFTest) | Test Program(C#) in WPF (using OpenGLView) |

## Platform

Since C++/CLI is used for C# bindings, and multiple DLL hacks are token for DLL-embedding, it's Windows-only.

VS2017(15.5.3) needed.

## Dependency

* [GLEW](http://glew.sourceforge.net/)  2.1.0

  [Modified BSD & MIT License](./License/glew.txt)

* [FreeGLUT](http://freeglut.sourceforge.net)  3.0.0

  [MIT License](./License/freeglut.txt)

* [boost](http://www.boost.org/)  1.66.0

  [Boost Software License](./License/boost.txt)

* [stb](https://github.com/nothings/stb)

  [MIT License](./License/stb.txt)

* [fmt](http://fmtlib.net) 4.1.0

  [BSD-2 License](./License/fmt.rst)

* [uchardet](https://www.freedesktop.org/wiki/Software/uchardet/) 0.0.6

  [MPL 1.1 License](./License/uchardet.txt)

* [FreeType2](https://www.freetype.org/) 2.8.1

  [The FreeType License](./License/freetype.txt)

* [cpplinq](http://cpplinq.codeplex.com/)

  [MS-PL License](./License/cpplinq.html)

* [date](https://howardhinnant.github.io/date/date.html) 2.4

  [MIT License](./License/date.txt)

## License

RayRenderer (including its component) is licensed under the [MIT license](License.txt).
