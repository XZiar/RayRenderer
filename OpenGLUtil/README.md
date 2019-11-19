# OpenGLUtil

A C++ wrapper of modern OpenGL.

It aims at providing a OOP wrapper which makes OpenGL's states transparent to upper programmers.

## Componoent

* **oglShader**  OpenGL Shader

  shader loading, data storeage, ext-property parsing.

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
  * oglTex3D -- any 2D texture
    * oglTex3DS -- immutable 3D texture
    * oglTex3DV -- 3D texture view (readonly)
  * oglTex2DArray -- 2D texture array (immutable only)
  * oglImg -- any image-load-store variables
    * oglImg2D -- 2D image variables
    * oglImg3D -- 3D image variables

* **oglVAO**  OpenGL Vertex attribute object
  
  Draw calls are finally fired by oglVAO

* **oglFBO**  OpenGL Framebuffer object
  * oglFBO2D -- 2D Framebuffer
  * oglFBO3D -- Layered Framebuffer

* **oglProgram**  OpenGL Program
  
  It's like a "shader" in other engine, with resources slot binding with uniforms, UBOs, textures, etc. 

* **oglContext**  OpenGL Context

  Wrapper for WGLContext(HGLRC/HDC) or GLXContext(GLXWindow,GLXDrawable). It provide message callback, context-sensitive resource management, and context-related state setting.
 
  It's important to keep track on what context this thread is using since object are shared between shared_contexts but bindings and some states are not.
  In fact objects has a weak-reference to its context(or shared-context base), and most operations need to check it in DEBUG mode.

  It's also important to make OpenGLUtil know current context, since it uses threadlocal to keep track of current context and is not aware of outside modification.

* **oglUtil**  OpenGL Utility
  
  It providing environment initializing and multi-thread worker.

## Dependency

* [OpenGL Header](../3rdParty/GL) 20191029

* [KHR API Header](../3rdParty/KHR) 20190424

* [3DBasic](../3DBasic)
  * Basic component -- providing basic 3d data structure

* [MiniLogger](../MiniLogger)
  
  used for logging message and errors, with prefix `OpenGLUtil`

* [ImageUtil](../ImageUtil)
  
  used for Image-Texture processing

* [AsyncExecutor](../AsyncExecutor)

  provide async-execution environment for OpenGL workers.

## Feature

### Wrap OpenGL function calls

OpenGL function calls (including GL 1.1) are all wrapped by OpenGLUtil and is context-based.

However, OpenGLUtil cannot control ouside calls to GL-functions, so try to seperate usage of internal and externel contexts and remember refresh state which switch context boundrary.

### Platform function support

OpenGLUtil provide wrapper of platform-related calls, support both WGL and GLX. It is set as threadlocal variable whenever tries to get current DeviceContext.

### DSA support

OpenGL provide **Direct State Access** functionality. [ARB](http://www.opengl.org/registry/specs/ARB/direct_state_access.txt) and [EXT](http://www.opengl.org/registry/specs/EXT/direct_state_access.txt) extensions are prioritily used, but Non-DSA function can be used to emulate DSA calls.

The DSA support is context-based. Some binding units (usually lower one) should be reserved for DSA emulation and other operations.

The DSA support is intended for internal use and is not exposed. It is set as threadlocal variable whenever tries to get current RenderContext.

### Shader

Some extensions are applied via defining attribute hints on comments. A single line started with `//@OGLU@` will be treated as an extension attribute.

The syntax is `//@OGLU@FuncName(arg0, arg1, ...)`, args can be number(treated as string) or string, where `""` will be removed for string.

#### Merged Shader

Despite of standard glsl shader like `*.vert`(Vertex Sahder), `*.frag`(Fragment Shader) and so on, OpenGLUtil also provid an extended shader format`*.glsl`.
It's main purpose is to merge multiple shader into one, which reduces redundant codes as well as eliminate bugs caused by careless.

`FuncName` should be `Stage`. Variadic arguments will be used to indicate what component this file should include(`VERT`, `FRAG`, `GEOM`...).

Merged Shader is based on glsl's preprocessor. For example, when compiling vertex shader, `OGLU_VERT` will be defined, so your codes can be compiled conditionally, while struct definition can be shared in any shader.

#### Uniform Description

`FuncName` should be `Property`. Arguments will be used to describe uniform variables. Its value is parsed but unused in OpenGLUtil, high-level program can use it.

The syntax is `//@OGLU@Property(UniformName, UniformType, [Description], [MinValue], [MaxValue])`. UniformName and UniformType mismatch an active uniform will be ignored.

`UniformType` can be one of the following: `COLOR`, `RANGE`, `VEC`, `INT`, `UINT`, `BOOL`, `FLOAT`.

`MinValue` and `MaxValue` is only useful when `UniformType` is a scalar type.

Uniform's initial value will be read after linked in oglProgram, but not all of them are supported.

#### Dynamic & Static Subroutine

OpenGL 4.0 introduced [**subroutine**](https://www.khronos.org/opengl/wiki/Shader_Subroutine), which provide easy runtime dispatch capability.

However, **subroutine** could be costly and will prevent optimization. Most upper shader would choose to conditional compiling using macro. 

OpenGLUtil provide a wrapper to cover both runtime dispatch and compile-time dispatch.

The syntax is `OGLU_ROUTINE(type, routinename, return-type, [arg, ...args])` and `OGLU_SUBROUTINE(type, funcname)`. The `OGLU_ROUTINE` defines both `subroutine type` and `subroutine uniform`, and `OGLU_SUBROUTINE` defines the `specific fucntion`. 

Normally, it's just a wrapper for OpenGL's subroutine. When a subroutine selection is statically specified in `ShaderConfig`, `routinename` become a macro of `funcname` and subroutine definition is gone. As a result, all usages of the subroutine will directly go to the specific function, while other selection functions can still be explicitly used.

#### Resource Mapping

Some common resources are widely used by shaders, so mapping is added to an vertex-attribute or uniform is a specific kind of resource.

`FuncName` should be `Mapping`. The syntax is `//@OGLU@Mapping(Type, VariableName)`, where `Type` is one of the following: 

#### DrawId

`DrawId` is useful when performing multi-draw or indirect rendering. It allows vertex shader to know which primitives it is drawing. However, `gl_DrawID` requires [`ARB_shader_draw_parameters`](https://www.khronos.org/opengl/wiki/Built-in_Variable_(GLSL)#Vertex_shader_inputs). For those who does not support that extension, common solution is to [use vertex attribute with divisor](https://www.g-truc.net/post-0518.html). 

OpenGLUtil provides a wrapper `ogluDrawId` to support both situation. When preparing the VAO, `VAOPrep` need to call `SetDrawId(prog)` even if `ARB_shader_draw_parameters` exists (when the extension exists, no target vertex attribute is defined so the operation will just be ignored).

#### glLayer

`gl_Layer` will be used for layered rendering, while using it in Fragment Shader requires glsl 4.3. For those version under 4.3, an varying variable will be provided to emulate it.

OpenGLUtil provides a wrapper `ogluLayer` to support both situation. In this case `ogluSetLayer(int)` shoud be called to properly set layerID.

### Resource Management

Texture and UBO are bound to slots, which are limited and context-sensitive. oglProgram(main shader) binds location with slots, which is context-insensitive. These two bindings are separated stored in context-related storage and program's state storage.

OpenGLUtil include an LRU-policy resource manager to handle slots bindings, which aims at least state-changing(binding texture is costly compared with uploading uniform(it may be buffered by driver)).

The manager handles at most 255 slots, and slot 0-3 are always reserved for other use(e.g, slot 0 is for default binding when uploading/downloading data), hence slot 4~N will be automatically managed.

For Texture, limit N is get by `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS`.
For UBO, limit N is get by `GL_MAX_UNIFORM_BUFFER_BINDINGS`.

LRU cache has 3 bi-direction linkedlist, `unused`,`used`,`fixed`. `unused` means available slots, `used` means bound slots, `unused` means pinned slots.

When an oglProgram is bound to current context, it binds its global states and pins them. Further bindings during draw calls will only modify `unused` and `used` list, so that states can be easily recovered after a draw call.

### Program State

Drawcalls are expensive. After wrapping, it's more expensive since state needed to be rollbacked.

In order to reduce binding state changing, LRU cache is used. But uniform values are not handled in that way.

Each time you call oglProgram's `draw()`, an `ProgDraw` is created with current global state. ProgDraw will restore program state after it's deconstructed, so **avoid creating too many `ProgDraw`**.

Setting value to uniform / binding ubo or tex resources in `ProgDraw` is temporal, but setting them in `ProgState` or `oglProgram` is global.

`ProgDraw` will enforce current context to use corresponding glProgram, and assumes that state kept until it is destroyed. In rder to prevent mistakes, `ProgDraw` will cause a spinlock in the thread, so more than one `ProgDraw` at the same time will hang the app.

### Multi-thread Worker

There is an optional worker, which can provide multi-thread operation ability. Operations inside are handled by AsyncExecutor, so only GL operations should be executed inside, leaving heavy task on other threads.

* Share Worker

  It has unique GL-context, but shared with object context(according to wgl's spec, buffer objects are shared but VAOs are not shared).
  
  It's useful for uploading/downloading data

* Isolate Worker

  It has unique GL-context, not shared with object context.
  
  It can be used to do some extra work just like another GL program, and data can be sent back to main context by invoking main-thread or invoking Share Worker

## License

OpenGLUtil (including its component) is licensed under the [MIT license](../License.txt).
