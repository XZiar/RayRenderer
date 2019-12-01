## TODO list for RayRenderer

* common
  - [x] Add linq (iterator-based)
  - [ ] Move Contorllable into standalone project with better metadata management
  - [ ] Add static control-item for Controllable
  - [ ] Implement new SIMD vector lib
  - [ ] Makes strcharset Linq-able
  - [x] Makes string split Linq-able

* SystemCommon
  - [ ] Add async file operation (cross-platform)
  - [ ] Add hugepage memory allocation

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
  - [ ] Re-design ResourceMappingManager
  - [ ] Add proper drawstate setting
  * TextureUtil
    - [x] Add mipmap
    - [ ] Migate ISPC tex-compressor to OpenCL

* OpenCLUtil
  - [ ] Remove explicit include of `cl*.h`
  - [x] Remove dependency of `OpenGLUtil`, seperate inter-op into new project

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
    - [ ] Add more import support


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
  - [ ] Move to .net core 3.0 with WPF/Winform
  - [ ] Add Vulkan backend
  - [ ] Add GLFW
  - [ ] Add FancyCL codegen which can write code in C++ with template/type-safe support and emit compute langs
  - [ ] Add custom window host (for OGL/vulkan)


