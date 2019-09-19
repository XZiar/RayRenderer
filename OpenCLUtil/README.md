# OpenCLUtil

A C++ wrapper of OpenCL.

It aims at providing a OOP wrapper which makes OpenCL's various object and functions.

## CL Dependency

* [OpenCL Header](../3rdParty/CL) 20190806 [License](../3rdParty/CL/LICENSE)

* [OpenCL-ICD-Loader](https://github.com/KhronosGroup/OpenCL-ICD-Loader) 2.2.4 [License](../3rdParty/OpenCL_ICD_Loader/LICENSE.txt)

## Componoent

* **oclPlatform**  OpenCL Platform

* **oclDevice**  OpenCL Device

* **oclContext**  OpenCL Context

  Created by oclPlatform with oclDevice. Context can be specified with support sharing with OpenGL(default enabled if device holds the GL support).

* **oclComQue**  OpenCL Command Queue

  Used to dispatch commands

* **oclMem**  OpenCL Memory-Like Object base
  
  Provide basic function for OpenCL's Memory-Like Object and inter-operation with OpenGL.

* **oclBuffer**  OpenCL Memory Object
  
  OpenCL memory

* **oclImage**  OpenCL Image Object
  
  OpenCL Image uses OpenGL's Texture data format

* **oclProgram**  OpenCL Program

  Provide argument setting and multi-dimension execution. Kernel's infomation can also be retrieved.

  Chained operation is partially supported (only single-link chain).

* **oclPromise**  OpenCL Promise

  With OpenCL's timer support.

* **oclUtil**  OpenCL Utility

## Dependency

* [common](../common)
  * Wrapper -- an extended version of STL's shared_ptr
  * Exception -- an exception model with support for nested-exception, strong-type, Unicode message, arbitrary extra data 
  * StringEx -- some useful operation for string, including encoding-conversion

* [OpenCL_ICD_Loader](../3rdParty/OpenCL_ICD_Loader)

  OpenCL ICD Loader.

* [OpenGLUtil](../OpenGLUtil)

  OpenGL Utility, used for optional inter-op but currently a required dependency.

* [MiniLogger](../MiniLogger)
  
  used for logging message and errors, with prefix `OpenCLUtil`

* C++17 required
  * optional -- used for some return value
  * tuple -- used for some internal structure

## License

OpenGLUtil (including its component) is licensed under the [MIT license](../License.txt).
