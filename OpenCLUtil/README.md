# OpenCLUtil

A C++ wrapper of OpenCL(1.2).

It aims at providing a OOP wrapper which makes OpenCL's various object and functions.

## Componoent

* **oclPlatform**  OpenCL Platform

* **oclDevice**  OpenCL Device

* **oclContext**  OpenCL Context

  Created by oclPlatform with oclDevice. Context can be specified with support sharing with OpenGL(default enabled if device holds the GL support).

* **oclComQue**  OpenCL Command Queue

  Used to dispatch commands

* **oclBuffer**  OpenCL Memory Object
  
  With GL inter-op

* **oglProgram**  OpenCL Program

  Provide argument setting and multi-dimension execution.

  No chained operation supported yet.

* **oglPromise**  OpenCL Promise

  With OpenCL's timer support.

* **oglUtil**  OpenCL Utility

## Dependency

* [common](../common)
  * Wrapper -- an extended version of stl's shared_ptr
  * Exception -- an exception model with support for nested-exception, strong-type, unicode message, arbitray extra data 
  * StringEx -- some useful operation for string, including encoding-conversion

* [OpenCL_ICD_Loader](../3rdParty/OpenCL_ICD_Loader)

  OpenCL ICD Loader.

* [OpenGLUtil](../OpenGLUtil)

  OpenGL Utility, used for inter-op but currently a required dependency.

* [miniLogger](../common/miniLogger)
  
  used for logging message and errors, with prefix `OpenCLUtil`

* C++17 required
  * optional -- used for some return value
  * tuple -- used for some internal structure

## License

OpenGLUtil (including its component) is licensed under the [MIT license](../License.txt).
