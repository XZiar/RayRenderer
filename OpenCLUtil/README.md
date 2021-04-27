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
  
  OpenCL image (uses ImageUtil's Texture data format)

* **oclProgram**  OpenCL Program

  Provide argument setting and multi-dimension execution. Kernel's infomation can also be retrieved.

* **oclKernel**  OpenCL Kernel

  Actual object that can be invoke. It's part of oclProgram and will retain program object.

  Chained operation is partially supported (only single-link chain).

* **oclPromise**  OpenCL Promise

  With OpenCL's timer support.

* **oclUtil**  OpenCL Utility

## Dependency

* [OpenCL Header](https://github.com/KhronosGroup/OpenCL-Headers) `submodule` 210426 [Apache 2.0 License](https://github.com/KhronosGroup/OpenCL-Headers/blob/master/LICENSE)
  
  C language headers for the OpenCL API.

* [OpenCL-ICD-Loader](https://github.com/KhronosGroup/OpenCL-ICD-Loader) `submodule` 3.0.1 [Apache 2.0 License]([../3rdParty/OpenCL-ICD_Loader/LICENSE](https://github.com/KhronosGroup/OpenCL-ICD-Loader/blob/master/LICENSE))

  Khronos official OpenCL ICD Loader.

* [ImageUtil](../ImageUtil)

  Provide texture/image format support

* [Nailang](../Nailang)

  NLCL language support

* [MiniLogger](../MiniLogger)
  
  Log message and errors, with prefix `OpenCLUtil`

## Requirements

OpenCLUtil currently requires at least OpenCL1.2

## License

OpenGLUtil (including its component) is licensed under the [MIT license](../License.txt).
