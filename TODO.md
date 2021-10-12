## TODO list for RayRenderer

* common
  - [x] Add linq (iterator-based)
  - [ ] Makes strcharset Linq-able
  - [x] Makes string split Linq-able
  - [x] Use dict to store Exception's custom info
  - [x] Use more compact custom container to replace deque in ResourceDict
  - [ ] Add enhanced enum (macro-based, string typing, parsing/stringify support, switchable, bitfield aseemble)
  * Parser
    - [ ] Parser should accept template for ContextReader, allowing custom reading strategy
  * Controllable
    - [ ] Move Contorllable into standalone project with better metadata management
    - [ ] Add static control-item for Controllable
    - [ ] Remove unnecessary lambda func for direct access
  * SIMD
    - [ ] Implement new vector lib with new SIMD
    - [x] Add compare capability with variable result requirement


* SystemCommon
  - [ ] Add async file operation (cross-platform)
  - [ ] Add hugepage memory allocation
  - [x] Seperate PromiseTask's functionalities of task-info and ret-value
  - [x] Move SIMD copy into SystemCommon
  - [ ] Seperate implementation of different SIMD into diff file with diff flags, try keep compatibility even with march=native
  - [x] Maybe move StringUtil into SystemCommon, enable OS-specific path
  - [x] Maybe move MiniLigger into SystemCommon, since commonlly used
  - [x] Maybe move AsyncExecutor into SystemCommon, since OS-related
  - [ ] Maybe move SpinLock into SystemCommon, enable HW&OS-specific waiting strategy
  - [x] Move common's exception into SystemCommon
  - [ ] Add high precision waitable condition varaible (WaitableTimer and nanosleep?)
  - [ ] Desgin common waitable, with native support of multi-wait
  * StringUtil
    - [x] Add general charset default value
    - [x] Add compile-time LE/BE decision
    - [ ] Applying SIMD acceleration 
    - [x] Merge fmt with better format-context support
    - [ ] Add BOM detection
    - [ ] Add custom opcode-based compile-time parsing&format support 
  * MiniLogger
    - [ ] Add pre-defined color arg
    - [ ] Add color filter to non-terminal backend (spin-lock based processing)
    - [ ] Use Pascal naming
    - [ ] Use new formating strategy, handle formating on worker thread and disable color on file output
  * AsyncExecutor
    - [x] Add support for returning value
    - [ ] Add passive executor, provide a way to handle task by outsider
    - [ ] Add multi thread executor
    - [ ] Embed boost.context, make boost.context a submodule

* ImageUtil
  - [x] Add memory input/output
  - [x] Make Image mutable
  - [x] Add stb as fallback handler
  - [x] Add more format suppprt
  - [ ] Use TexFormat instead of ImageDataType
  - [ ] Add DataType-based support query
  - [ ] Throw proper exception when facing error
  - [ ] Add external decoder/encoder (eg, intel media sdk)
  - [ ] Move DataTypeConvertor to cpp, with runtime path selection
  - [ ] Add test for DataTypeConvertor

* WindowHost
  - [ ] Add support for direct image blit (for offscreen rendering)
  - [ ] Add support for wayland
  - [ ] Add support for iOS (Cocoa Touch)
  - [ ] Add support for gesture
  - [ ] Add support for touch screen logic
  - [ ] Add support for remote redering on Android (with IPC and shares ANativeWindow?)
  - [ ] Add multi-backend support
  - [ ] Add vsync signal support
  - [ ] Add Icon support
  - [ ] Add timer support

* Nailang
  - [x] Re-design operators handling to support `short-circuit evaluation` and `assign if null`
  - [x] Add genenral datatype for `Arg` for type-checking
  - [ ] Move to utf8 (save mem and closer to output type)
  - [x] Shrink memory usage of Arg
  - [x] Make parser throw proper exception
  - [x] Embed position information in token
  - [ ] Add `include` support
  - [ ] Add python parser & runtime
  - [x] Explicitly handle loops
  - [ ] Fix test for nested-scope arg access
  - [x] Add proper stacktrace when throw exception
  - [x] Add nested brace support for ReplaceEngine
  - [x] Move arg/func's lookup logic into StackFrame
  - [x] Pre-compute scope when parsing, for arg and func name
  - [x] Add array type
  - [x] Add array access by `[]`
  - [x] Allow fake arg when try to set value of an arg
  - [x] Add register-based customvar handler
  - [x] Evaluate `Expr`(or `RawArg`) before calling the function
  - [x] Add ternary operator
  - [x] CustomVar also report `TypeData` for easier type conversion
  - [x] Add capture support when define func
  - [x] Seperate Executor and Runtime
  - [x] Correct handle *nary expression when facing custom object
  - [x] Seperate `AutoVar` and `AutoVar[]` to allow custom info when accessing AutoVar 
  - [x] Add bitwise operator support
  - [ ] Add stack-like EvaluateContext, put all args into the same container (since no way to add var into prev caller)
  - [ ] Correctly handle self assign for customvar (for efficiency)
  - [ ] Add member func call for arg by `()`
  - [ ] Remove PartedName, func will only keep fullname
  - [ ] Prefer member func than global func, use query to replace PartedName
  - [ ] Translate Indexer to member func, and only keeps subfield to be chainable.
  - [ ] Add serialize/deserialize to/from json (with mempool)
  - [ ] Embed comment infomation into statement
  - [ ] Export functionality for debugging
  - [ ] Add external GUI dubugger

* XCompute
  - [ ] Merge OpenCLInterop
  - [ ] Add compatible layer for shader sharing between NLCL and NLGL
  - [ ] Add compatible layer for shader sharing between NLCL and NLDX
  - [ ] Add interop between opencl and directx (delay load + conditional compiling)
  - [x] Add support of output debug data into SpreadsheetML format
  - [x] Add common syntax to define args of the instance
  - [x] Add common syntax to define structure
  - [x] Expose common API to inject custom extensions
  - [ ] Add support for matrix datatype
  - [ ] Write program completely in XCNL, generate native code based on AST with correct type tracking and conversion. 
  - [ ] Add common interface for texture/buffer, SRV/UAV
  - [ ] Add design to seperate program and parameter for VK/DX. Add abstraction of PSO for GL, use delay bind for CL.
  - [ ] Make better design with prefix deprendency, prefix generator should be able to register multiple dependencies

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
  - [ ] Add shader include support (file lookup management)
  - [ ] Allow disable feature via env or manually to allow compatiblilty test
  - [ ] Move VAO's prepare before returning actual VAO
  - [ ] Move to use threadpool to provide 1:1 context support, add cmdlist to record api calls and use IPC to execute remotely
  - [ ] Direct load ICD lib to support multi-device/context
  * NLGL
    - [ ] Add basic NLGL
    - [ ] Add proper drawstate setting with NLGL
    - [ ] Replace preprocessor-based ExtShader to NLGL-based

* OpenCLUtil
  - [x] Remove explicit include of `cl*.h`
  - [x] Query functions explicitly to support both ICD Loader and non-icd platform
  - [x] Remove dependency of `OpenGLUtil`, seperate inter-op into new project
  - [x] Add compatibility for OpenCL verson less than 1.2
  - [x] Make oclKernel associate with specific device
  - [x] Make oclPromise directly use cl_event for better management
  - [ ] Mimic FillBuffer using embeded kernel for pre-1.2
  - [ ] Add support for Intel's accelerator
  - [ ] Make device inherit with extra information
  * NLCL
    - [x] Add extended debugging output using NLCL
    - [x] Mimic subgroup shuffle on non-intel
    - [x] Mimic subgroup shuffle on unsupported platform (nv)
    - [x] Use extension to provide debug support
    - [x] Add dp4a extension
    - [x] Add better kernel argument info parsing in NLCL
    - [x] Seperate into shared compute library

* DirectXUtil
  - [ ] Add PCIE bus query
  - [ ] Add compatible dx11 layer creation
  - [ ] Add graphic pipeline
  - [ ] Add metric query support (profiling)
  - [ ] Add texture support
  - [ ] Add sampler support
  - [ ] Add shared desc heap
  - [ ] Move to dx12 namespace and create similar framework for dx11 in dx11 namespace (can share some common struct)
  * NLDX
    - [x] Add basic NLDX
    - [ ] Mimic subgroup on platform without Wave support
    - [ ] Add Debug support

* TextureUtil
  - [x] Add mipmap
  - [ ] Add blur
  - [ ] Migrate ISPC tex-compressor to OpenCL
  - [ ] Move to based on XCompute with multi-language support

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
    - [ ] Seperate renderer with support of offscreen rendering and multi-backend
  * Rendering
    - [ ] Add Cubemap (environment map) with HDR
    - [ ] Implement IBL
    - [ ] Add ShadowMapping
    - [ ] Add Raytracing
    - [ ] Add Deffered rendering
  * Model
    - [ ] Add skeleton animation
    - [ ] Add more import format support
  * CLI Wrapper
    - [ ] Remove incompatibility with C++20

* WPFTest
  - [x] Add async-loader of Image for thumbnail
  - [ ] Change to move convinient mouse+keyboard operation
  - [ ] More buttons and proper icon
  - [x] Develop AnyDock
  - [ ] Fix scrollview's crash when switching to other selection

* AnyDock
  - [ ] Replace with DockPanel to provide better Dock State

* Build
  - [x] Custom build procedure for Linux
  - [x] Generate build Makefile from python script and project-info json
  - [x] Add Pch for GCC build
  - [x] Try seperate vcxproj and source file, for 3rdparty lib project using submodule
  - [x] Add better support for other build targets on ARM
  - [ ] Add support of different build flags for specific file 
  - [ ] Move to fully self-controlled build system
  - [ ] Add pseudo project, provide include-path info or install info or linkage info 

* Global
  - [x] Port some utilities to Linux 
  - [x] Port core components to Linux 
  - [ ] Move to .net core 6 with WPF/Winform
  - [ ] Move to avalonia as GUI
  - [ ] Add Vulkan backend
  - [x] Add custom window host (for OGL/vulkan)
  - [ ] Seperate OGL/DX/Vulkan backend
  - [ ] Add remote rendering backend (out-of-process or network-based)
  - [x] Add custom base DSL for OpenGL and OpenCL
  - [x] Switch to github actions for CI?
  - [x] Repalce cryptopp with lightweighted digest library (maybe setup a new project)
  - [x] Embed gtest for latest fixes for C++20
  - [x] Move to submodule-based 3rdParty libraries
  - [x] Add basic DX12 runtime wrapper
  - [ ] Adopt `std::launder` when necessary to reduce UB
  - [ ] Seperate sub projects into different repos

