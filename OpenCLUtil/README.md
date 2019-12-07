# OpenCLUtil

A C++ wrapper of OpenCL.

It aims at providing a OOP wrapper which makes OpenCL's various object and functions.

## Componoent

* **oclPlatform**  OpenCL Platform

* **oclDevice**  OpenCL Device

* **oclContext**  OpenCL Context

  Created by oclPlatform with oclDevice. Context can be specified with support sharing with OpenGL(default enabled if device holds the GL support).

* **oclComQue**  OpenCL Command Queue

  Used to dispatch commands

* **oclMem**  OpenCL Memory-Like Object base
  
  Provide basic function for OpenCL's Memory-Like Object.

* **oclBuffer**  OpenCL Memory Object
  
  OpenCL memory

* **oclImage**  OpenCL Image Object
  
  OpenCL Image uses OpenGL's Texture data format

* **oclProgram**  OpenCL Program

  Provide argument setting and multi-dimension execution. Kernel's infomation can also be retrieved.

  Chained operation is partially supported (only single-link chain).

* **oclKernel**  OpenCL Kernel

  Actual object that can be invoke. It's part of oclProgram and will retain program object. Any build operation will invalid previous kernel.

* **oclPromise**  OpenCL Promise

  With OpenCL's timer support.

* **oclUtil**  OpenCL Utility

## Dependency

* [OpenCL Header](../3rdParty/CL) 20190806 [License](../3rdParty/CL/LICENSE)

* [OpenCL-ICD-Loader](https://github.com/KhronosGroup/OpenCL-ICD-Loader) 2.2.5 [License](../3rdParty/OpenCL/LICENSE)

  OpenCL ICD Loader.

* [common](../common)
  * Exception -- an exception model with support for nested-exception, strong-type, Unicode message, arbitrary extra data 
  * StringEx -- some useful operation for string, including encoding-conversion

* [MiniLogger](../MiniLogger)
  
  used for logging message and errors, with prefix `OpenCLUtil`

## Requirements

OpenCLUtil currently requires at least OpenCL1.2

## License

OpenGLUtil (including its component) is licensed under the [MIT license](../License.txt).
