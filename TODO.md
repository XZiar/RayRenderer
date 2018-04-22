## TODO list for RayRenderer

* AsyncExecutor
  - [ ] Add support for returning a task/promise

* OpenGLUtil
  - [ ] Add mipmap
  - [ ] Implement FrameBuffer
  - [ ] Better workaround for copying compressed texture
  - [ ] Add RenderDoc support (conditionally remove DSA_EXT, or fork RenderDoc)
  - [x] Add Intel's ISPC tex-compressor as texture compresser
  - [ ] Add debugging support

* FontHelper
  - [ ] Implement proper text render
  - [ ] Add text render properties

* RenderCore
  - [x] Implement SpotLight
  - [ ] Implement IBL
  - [ ] Add convertion between BlinnPhongMaterial and PBRmaterial
  - [ ] Update blinn-phong shader, or combine them
  - [ ] Move normal mapping into VS stage
  * Core function
    - [ ] Add ShadowMapping
    - [ ] Add Raytracing
    - [ ] Add multi-pass
    - [ ] Add Deffered rendering
  * Model
    - [ ] Add skeleton animation
    - [ ] Add more import support

* WPFTest
  - [ ] Change to move convinient mouse+keyboard operation
  - [ ] More buttons and proper icon

* Global
  - [ ] Add GacUI 