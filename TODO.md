## TODO list for RayRenderer

* common
  - [ ] Add async file operation (cross-platform)
  - [x] Add linq (iterator-based)
  - [ ] Move Contorllable into standalone project with better metadata management
  - [ ] Implement new SIMD vector lib.

* AsyncExecutor
  - [x] Add support for returning value
  - [ ] Add multi thread executor

* ImageUtil
  - [x] Add memory input/output
  - [x] Make Image mutable
  - [x] Add stb as fallback handler
  - [x] Add more format suppprt
  - [ ] Throw proper exception when facing error

* OpenGLUtil
  - [x] Implement FrameBuffer
  - [ ] Better workaround for copying compressed texture
  - [x] Add Intel's ISPC tex-compressor as texture compresser
  - [ ] Add debugging support
  - [x] Move Camera component out of OpenGLUtil (shouldn't handle here)
  - [x] Combine subroutine with macro-based static dispatch
  - [x] Add context-sesative dispatching for version-based functions 
  - [ ] Seperate sampler and texture
  - [ ] Seperate VAO bindings
  - [ ] Remove explicit include of glew
  - [ ] Reduce hazzlement of TexDataType and TexInnerType
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
  - [ ] Replace glew since it lacks support for multi-thread & multi-contxt
  - [ ] Add GLFW
  - [ ] Add static control-item for Controllable