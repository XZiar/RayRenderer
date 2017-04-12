RayRenderer
===========================

A simple renderer based on my old RayTrace project.

It will contain mini version of BLAS library, C++ wrapper for OpenGL and GLUT, C#(WPF) GUI  

### Changes from old project

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
| [miniLogger](./common/miniLogger) | mini logger |
| [OpenGLUtil](./OpenGLUtil) | Wrapper of OpenGL things |
| [OpenCLUtil](./OpenCLUtil) | Wrapper of OpenCL things |
| [FontHelper](./FontHelper) | Helper for displaying font in OpenGL |
| [FreeGLUTView](./FreeGLUTView) | Wrapper of FreeGLUT |
| [OpenGLView](./OpenGLView) | Wrapper of OpenGL window in WinForm |
| [RenderCore](./RenderCore) | Core of RayRender |
| [RenderCoreWrap](./RenderCoreWrap) | C# Wrapper for RayRender core |
| [GLUTTest](./GLUTTest) | Test Program(C++) (using FreeGLUTView) |
| [WinFormTest](./WinFormTest) | Test Program(C#) in WinForm (using OpenGLView) |
| [WPFTest](./WPFTest) | Test Program(C#) in WPF (using OpenGLView) |


## Dependency

* [GLEW](http://glew.sourceforge.net/)  2.0.0

  [Modified BSD & MIT License](./License/glew.txt)

* [FreeGLUT](http://freeglut.sourceforge.net)  3.0.0

  [MIT License](./License/freeglut.txt)

* [boost](http://www.boost.org/)  1.64-beta2

  [Boost Software License](./License/boost.txt)

* [stb](https://github.com/nothings/stb)

  [MIT License](./License/stb.txt)

* [fmt](http://fmtlib.net) 3.0.1

  [BSD-2 License](./License/fmt.rst)

## License

RayRenderer (including its component) is licensed under the [MIT license](License.txt).
