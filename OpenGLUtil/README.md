# OpenGLUtil

A C++ wrapper of modern OpenGL.

It aims at providing a OOP wrapper which makes OpenGL's states transparent to upper programmers.

## Componoent

* **oglShader**  OpenGL Shader

  shader loadin, data storeage, ext-property parsing.

* **oglBuffer**  OpenGL Buffer objects
  * oglVBO -- array buffer object
  * oglTBO -- texture buffer object
  * oglUBO -- unifrom buffer object
  * oglIBO -- indirect command buffer object
  * oglEBO -- element buffer object (indexed element buffer)

* **oglTexture**  OpenGL Texture
  * oglTex2D -- any 2D texture
    * oglTex2DS -- immutable 2D texture
    * oglTex2DD -- mutable 2D texture
    * oglTex2DV -- 2D texture view (readonly)
  * oglTex2DArray -- 2D texture array (immutable only)

* **oglVAO**  OpenGL Vertex attribute object
  
  Draw calls are finally fired by oglVAO

* **oglProgram**  OpenGL Program
  
  It's like a "shader" in other engine, with resources slot binding with uniforms, UBOs, textures, etc. 

* **oglContext**  OpenGL Context

  Simple wrapper for WGLContext(HGLRC). It provide message callback and context-sensative resource management.
 
  It's important to keep track on what context this thread is using since object are shared between shared_contexts but bindings and some states are not.

* **oglUtil**  OpenGL Utility
  
  It providing environment initializing and multi-thread worker.

## Dependency

* [common](../common)
  * Wrapper      -- an extended version of stl's shared_ptr
  * Exception    -- an exception model with support for nested-exception, strong-type, unicode message, arbitray extra data 
  * StringEx     -- some useful operation for string, including encoding-conversion
  * ContainerEx  -- some useful operation for finding in containers, and self-contained map.

* [3DBasic](../3DBasic)
  * Basic component -- providing basic 3d data structure
  * Camera -- an uvn camera

* [glew](../3rdParty/glew)

  low-level OpenGL API supportor.

* [miniLogger](../common/miniLogger)
  
  used for logging message and errors, with prefix `OpenGLUtil`

* [ImageUtil](../ImageUtil)
  
  used for Image-Texture processing

* [AsyncExecutor](../common/AsyncExecutor)

  provide async-execution environment for OpenGL workers.

* C++17 required
  * optional -- used for some return value
  * variant  -- used for uniform value storage
  * any      -- used for uniform value storage
  * tuple    -- used for some internal structure

## Feature

### Shader

#### Merged Shader

Despite of standard glsl shader like `*.vert`(Vertex Sahder), `*.frag`(Fragment Shader) and so on, OpenGLUtil also provid an extended shader format`*.glsl`.
It's main purpose is to merge multiple shader into one, which reduces redundent codes as well as eliminate bugs caused by careless.

A single line strted with `//@@$$` will be used to indicate what component this file should include(`VERT`,`FRAG`,`GEOM`...separated with `|` ).

Extended Shader is based on glsl's preprocessor. For example, when compiling vertex shader, `OGLU_VERT` will be defined, so your codes can be compiled conditionally, while struct definition can be shared in any shader.

#### Uniform Description

A single line started with `//@@##` will be used to describe uniform variables. Its value is parsed but unused in OpenGLUtil, high-level program can use it.

The stynx is `//@@##{UniformName}|{UniformType}|[Description]|[MinValue]|[MaxValue]`. UniformName and UniformType mismatch an active uniform will be ignored.

`UniformType` can be one of the following: `COLOR`, `RANGE`, `VEC`, `INT`, `UINT`, `BOOL`, `FLOAT`.

`MinValue` and `MaxValue` is only useful when `UniformType` is a scalar type.

Uniform's initial value will be readed after linked in oglProgram, but not all of them are supported.

#### Resource Mapping

Some common resources are widely used by shaders, so mapping is added to an vertex-sttribute or uniform is a specific kind of resource.

The syntax is `//@@->{Type}|{VariableName}`, where `Type` is one of the following: 

`ProjectMat`, `ViewMat`, `ModelMat`, `MVPMat`, `MVPNormMat`, `CamPosVec`, `VertPos` for uniforms.

`VertNorm`, `VertTexc`, `VertColor`, `VertTan` for vertex attributes.

### Resource Management

Texture and UBO are binded to slots, which are limited and context-sensative. oglProgram(main shader) binds location with slots, which is context-insensative. These two bindings are seperated stored in context-related storage and program's state storage.

OpenGLUtil include an LRU-policy resource manager to handle slots bindings, which aims at least state-changing(binding texture is costy compared with uploading uniform(it may be buffered by driver)).

The manager handles at most 255 slots, and slot 0-3 are always reserved for other use(e.g, slot 0 is for default binding when uploading/dowloading data), hence slot 4~N will be automatically managed.

For Texture, limit N is get by `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS`.
For UBO, limit N is get by `GL_MAX_UNIFORM_BUFFER_BINDINGS`.

LRU cache has 3 bi-direction linkedlist, `unused`,`used`,`fixed`. `unused` means avaliable slots, `used` means binded slots, `unused` means pinned slots.

When an oglProgram is binded to current context, it binds its global states and pins them. Further bindings during draw calls will only modify `unused` and `used` list, so that states can be easily recovered after a draw call.

### Program State

Drawcalls are expensive. After wrapping, it's more expensive since state needed to be rollbacked.

In order to reduce binding state changing, LRU cache is used. But uniform values are not handled in that way.

Eachtime you call oglProgram's `draw()`, an `ProgDraw` is created with current global state. ProgDraw will restore program state after it's deconstructed, so avoid creating too many `ProgDraw`.

Setting value to uniform / binding ubo or tex resources in `ProgDraw` is temporal, but setting them in `ProgState` or `oglProgram` is global.

`ProgDraw` will enforce current context to use corresponding glProgram, and assumes that state kept until it is destroyed. So avoid keeping more than one `ProgDraw` at the same time in a single thread.

### Multi-thread Worker

There are two workers inside OpenGLUtil, proving multi-thread operation ability. Operations inside should be minimized, leaving heavy task on other threads.

Both of them are under async environment, so multiple tasks can be executed in a single thread. Tasks' waiting should be handled by AsyncAgent. OpenGL-Sync promise can be obtained for operation sync.

* Sync Worker

  It has unique GL-context, but shared with main context(Acording to wgl's spec, buffer objects are shared but VAOs are not shared).
  
  It's useful for uploading/dowloading data

* Async Worker

  It has unique GL-context, not shared with main context.
  
  It can be used to do some extra work just like another gl program, and data can be sent back to main context by invoking main-thread or invoking Sync Worker

## License

OpenGLUtil (including its component) is licensed under the [MIT license](../License.txt).
