# OpenCLInterop

Providing OpenCL's interoperation with other components.

It currently only provide support with OpenGL.

## Componoent

* **GLInterop**  Interoperation with OpenGL

* **oclGLBuffer**  Actual oclBuffer with a OpenGL buffer as backend

* **oclGLInterBuf**  Holder that holds interop resources for oclBuffer and oglBuf

* **oclGLImg2D**, **oclGLImg3D**  Actual oclImg with a OpenGL texture as backend

* **oclGLInterImg2D**, **oclGLInterImg3D**  Holder that holds interop resources for oclImg and oglTex

## Dependency

* [OpenCL-ICD-Loader](https://github.com/KhronosGroup/OpenCL-ICD-Loader) 2.2.4 [License](../3rdParty/OpenCL/LICENSE)

  OpenCL ICD Loader.

* [OpenGLUtil](../OpenGLUtil)
  
  provide OpenGL object.
  
* [OpenCLUtil](../OpenCLUtil)
  
  Provide OpenCL Object.

* [MiniLogger](../MiniLogger)
  
  used for logging message and errors, with prefix `OpenCLUtil`

## License

OpenGLInterop (including its component) is licensed under the [MIT license](../License.txt).
