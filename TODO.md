## TODO list for RayRenderer

* common
  - [x] Add linq (iterator-based)
  - [ ] Implement new SIMD vector lib
  - [ ] Makes strcharset Linq-able
  - [x] Makes string split Linq-able
  - [ ] Use dict to store Exception's custom info
  - [ ] Add enhanced enum (macro-based, string typing, parsing/stringify support, switchable, bitfield aseemble)
  * Parser
    - [ ] Parser should accept template for ContextReader, allowing custom reading strategy
  * Controllable
    - [ ] Move Contorllable into standalone project with better metadata management
    - [ ] Add static control-item for Controllable
    - [ ] Remove unnecessary lambda func for direct access

* StringCharset
  - [x] Add general charset default value
  - [x] Add compile-time LE/BE decision
  - [ ] Applying SIMD acceleration 
  - [ ] Merge fmt with better format-context support
  - [ ] Add BOM detection

* SystemCommon
  - [ ] Add async file operation (cross-platform)
  - [ ] Add hugepage memory allocation
  - [ ] Add SpinLock implementation

* AsyncExecutor
  - [x] Add support for returning value
  - [ ] Add multi thread executor

* ImageUtil
  - [x] Add memory input/output
  - [x] Make Image mutable
  - [x] Add stb as fallback handler
  - [x] Add more format suppprt
  - [ ] Use TexFormat instead of ImageDataType
  - [ ] Add DataType-based support query
  - [ ] Throw proper exception when facing error
  - [ ] Add external decoder/encoder (e.g, intel media sdk)
  - [ ] Move DataTypeConvertor to cpp, with runtime path selection
  - [ ] Add test for DataTypeConvertor

* OpenGLUtil
  - [x] Implement FrameBuffer
  - [ ] Better workaround for copying compressed texture
  - [x] Add Intel's ISPC tex-compressor as texture compresser
  - [x] Add debugging support (renderdoc)
  - [x] Move Camera component out of OpenGLUtil (shouldn't handle here)
  - [x] Combine subroutine with macro-based static dispatch
  - [x] Add context-sesative dispatching for version-based functions 
  - [ ] Seperate sampler and texture
  - [ ] Seperate VAO bindings
  - [x] Remove explicit include of glew
  - [x] Reduce hazzlement of TexDataType and TexInnerType
  - [x] Re-design object creation
  - [x] Replace glew since it lacks support for multi-thread & multi-contxt
  - [x] Re-design ResourceMappingManager
  - [x] Emulate subroutine on unsupported platform
  - [x] Add bindless texture support
  - [ ] Replace preprocessor-based ExtShader to NLGL-based
  - [ ] Add proper drawstate setting with NLGL
  - [ ] Add shader include support (file lookup management)
  - [ ] Allow disable feature via env or manually to allow compatiblilty test
  - [ ] Move VAO's prepare before returning actual VAO

* OpenCLUtil
  - [ ] Remove explicit include of `cl*.h`
  - [x] Remove dependency of `OpenGLUtil`, seperate inter-op into new project
  - [x] Add compatibility for OpenCL verson less than 1.2.
  - [x] Add extended debugging output using NLCL
  - [x] Make oclKernel associate with specific device
  - [x] Make oclPromise directly use cl_event for better management
  - [ ] Mimic subgroup shuffle on non-intel
  - [ ] Mimic subgroup on unsupported platform (nv)
  - [ ] Mimic FillBuffer using embeded kernel for pre-1.2
  - [ ] Add better kernel argument info parsing in NLCL
  - [ ] Add support for Intel's accelerator

* OpenCLInterop
  - [ ] Add compatible layer for shader sharing between NLCL and NLGL

* TextureUtil
    - [x] Add mipmap
    - [ ] Add blur
    - [ ] Migate ISPC tex-compressor to OpenCL

* FontHelper
  - [ ] Implement proper text render
  - [ ] Add text render properties

* RenderCore
  - [x] Implement SpotLight
  - [ ] Update blinn-phong shader, or combine them
  - [ ] Move normal mapping into VS stage
  - [ ] Add proper deserialization
  - [x] Seperate color correction pass using 3D LUT
  - [ ] Add click check(hittest)
  * Architecture
    - [ ] Design plugin system, provide common interface
    - [ ] Redesign shader-drawable's binding strategy
    - [x] Add pass concept (customizable)
    - [x] Add render pipeline concept (layers of passes)
    - [ ] Add batch of drawables
  * Rendering
    - [ ] Add Cubemap (environment map) with HDR
    - [ ] Implement IBL
    - [ ] Add ShadowMapping
    - [ ] Add Raytracing
    - [ ] Add Deffered rendering
  * Model
    - [ ] Add skeleton animation
    - [ ] Add more import format support

* Nailang
  - [x] Re-design operators handling to support `short-circuit evaluation` and `assign if null`
  - [x] Add genenral datatype for `Arg` for type-checking
  - [ ] Add array type
  - [ ] Shrink memory usage of Arg
  - [x] Make parser throw proper exception
  - [ ] Embed position information in token
  - [ ] Add `include` support
  - [ ] Add python parser & runtime

* WPFTest
  - [x] Add async-loader of Image for thumbnail
  - [ ] Change to move convinient mouse+keyboard operation
  - [ ] More buttons and proper icon
  - [x] Develop AnyDock
  - [ ] Fix scrollview's crash when switching to other selection

* AnyDock
  - [ ] Repalce with DockPanel to provide better Dock State

* Build
  - [x] Custom build procedure for Linux
  - [x] Generate build Makefile from python script and project-info json
  - [x] Add Pch for GCC build
  - [ ] Move to fully self-controlled build system

* Global
  - [ ] Add GacUI
  - [x] Port some utilities to Linux 
  - [x] Port core components to Linux 
  - [ ] Move to .net core 3.1 with WPF/Winform
  - [ ] Add Vulkan backend
  - [ ] Add GLFW
  - [ ] Add FancyCL codegen which can write code in C++ with template/type-safe support and emit compute langs
  - [ ] Add custom window host (for OGL/vulkan)
  - [ ] Seperate OGL/Vulkan backend
  - [ ] Add remote rendering backend (out-of-process or network-based)
  - [x] Add custom base DSL for OpenGL and OpenCL
  - [ ] Switch to github actions for CI?
  - [x] Repalce cryptopp with lightweighted digest library (maybe setup a new project)
  - [ ] Embed gtest for latest fixes for C++20

