## TODO list for RayRenderer

* common
  - [ ] Add async file operation (cross-platform)

* AsyncExecutor
  - [x] Add support for returning value
  - [ ] Add multi thread executor

* ImageUtil
  - [ ] Add memory input/output
  - [x] Make Image mutable
  - [ ] Add stb as fallback handler
  - [ ] Add more format suppprt

* OpenGLUtil
  - [ ] Add mipmap
  - [x] Implement FrameBuffer
  - [ ] Better workaround for copying compressed texture
  - [ ] Add RenderDoc support (conditionally remove DSA_EXT, or fork RenderDoc)
  - [x] Add Intel's ISPC tex-compressor as texture compresser
  - [ ] Add debugging support
  - [x] Move Camera component out of OpenGLUtil (shouldn't handle here)
  - [x] Combine subroutine with macro-based static dispatch
  - [x] Add context-sesative dispatching for version-based functions 

* FontHelper
  - [ ] Implement proper text render
  - [ ] Add text render properties

* RenderCore
  - [x] Implement SpotLight
  - [ ] Implement IBL
  - [ ] Update blinn-phong shader, or combine them
  - [ ] Move normal mapping into VS stage
  - [ ] Add proper deserialization
  - [ ] Design plugin system, provide common interface
  - [x] Seperate color correction pass using 3D LUT
  * Core function
    - [ ] Add ShadowMapping
    - [ ] Add Raytracing
    - [ ] Add multi-pass workflow
    - [ ] Add Deffered rendering
  * Model
    - [ ] Add skeleton animation
    - [ ] Add more import support


* WPFTest
  - [ ] Change to move convinient mouse+keyboard operation
  - [ ] More buttons and proper icon
  - [ ] Use xctk's PropertyGrid as editor layout, with Controllable

* Global
  - [ ] Add GacUI
  - [x] Port some utilities to Linux 
  - [ ] Port core components to Linux 
  - [x] Custom build procedure for Linux
  - [ ] Real custom build system similar to Scons, get rid of makefiles
  - [ ] Replace glew since it lacks support for multi-thread & multi-contxt
  - [ ] Add Pch for Linux build