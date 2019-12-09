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
  * oglDrawProgram -- Program which runs on graphics pipeline
  * oglcomputeProgram -- Program which runs on compute pipeline

* **oglContext**  OpenGL Context

  Wrapper for WGLContext(HGLRC/HDC) or GLXContext(GLXWindow,GLXDrawable). It provide message callback, context-sensitive resource management, and some context-related state setting.
 
  It's important to keep track on what context this thread is using since object are shared between shared_contexts but bindings and some states are not.
  In fact objects has a weak-reference to its context(or shared-context base), and most operations will check if it is in a compatible context(maybe shared) when under DEBUG mode.

  It's also important to make OpenGLUtil know current context, since it uses threadlocal to keep track of current context and is not aware of outside modification.

  oglContext exposes current context's capabilities via `Capability` member.

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

* [boost.stacktrace](../3rdParty/boost.stacktrace)

  provide detail stacktrace when throwing exceptions

## Requirements

Although OpenGLUtil tries to cover difference between versions and try to provide as much functionality as possible, there's still some minimum requirements to allow it works correctly.

* ([WGL_ARB_extensions_string](https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_extensions_string.txt) or [WGL_EXT_extensions_string](https://www.khronos.org/registry/OpenGL/extensions/EXT/WGL_EXT_extensions_string.txt)) (Windows), ([GLX_ARB_get_proc_address](https://www.khronos.org/registry/OpenGL/extensions/ARB/GLX_ARB_get_proc_address.txt)) (*nux) required.
* [WGL_ARB_create_context](https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt) (Windows), [GLX_ARB_create_context](https://www.khronos.org/registry/OpenGL/extensions/ARB/GLX_ARB_create_context.txt) (*nix) required.
* [GL_ARB_program_interface_query](https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_program_interface_query.txt) required for oglProgram, since it tries to introspect program's info.
* [ARB_separate_shader_objects](https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_separate_shader_objects.txt) required for oglProgram, since it uses `glProgramUniform` to get/set uniforms
* GL3.0 required for Core Profile Context.

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

Merged Shader is based on glsl's preprocessor. For example, when compiling vertex shader, `OGLU_VERT` will be defined just after the version statement, so your codes can be compiled conditionally, while struct definition can be shared in any shader.

#### Uniform Description

`FuncName` should be `Property`. Arguments will be used to describe uniform variables. Its value is parsed but unused in OpenGLUtil, high-level program can use it.

The syntax is `//@OGLU@Property(UniformName, UniformType, [Description], [MinValue], [MaxValue])`. UniformName and UniformType mismatch an active uniform will be ignored.

`UniformType` can be one of the following: `COLOR`, `RANGE`, `VEC`, `INT`, `UINT`, `BOOL`, `FLOAT`.

`MinValue` and `MaxValue` is only useful when `UniformType` is a scalar type.

Uniform's initial value will be read after linked in oglProgram, but not all of them are supported.

#### Dynamic & Emulate & Static Subroutine

OpenGL 4.0 introduced [**subroutine**](https://www.khronos.org/opengl/wiki/Shader_Subroutine), which provide easy runtime dispatch capability.

However, **subroutine** could be costly and will prevent optimization. Most upper shader would choose to conditional compiling using macro. Also, some old platform does not support it, although it has been wide emulated using id+switch.

OpenGLUtil provide a wrapper to cover all these three solution: **runtime dispatch**, **emulate subroutine** and **compile-time dispatch**.

The syntax is `OGLU_ROUTINE(type, routinename, return-type, [arg, ...args])` and `OGLU_SUBROUTINE(type, funcname)`. The `OGLU_ROUTINE` defines both `subroutine type` and `subroutine uniform`, and `OGLU_SUBROUTINE` defines the `specific fucntion`. 

Normally, it's just a wrapper for OpenGL's subroutine. When a subroutine selection is statically specified in `ShaderConfig`, `routinename` become a macro of `funcname` and subroutine definition is gone. As a result, all usages of the subroutine will directly go to the specific function, while other selection functions can still be explicitly used.

If there's any dynamic subroutine, a request of extension `GL_ARB_shader_subroutine` will be placed into the shader.

If the runtime detectes that context does not support subroutine, it will generate selection uniform and wrapper function with switches to emulate the functionality. The id of subroutine's routine will be statically assigned using their line number. It is possible for compiler to optimize out the uniform, so weather the subroutine is active is still decided by the OpenGL runtime. 

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

OpenGLUtil include a cached resource manager to handle slots bindings, which aims at least state-changing (binding texture is costly compared to uploading uniforms).

The manager handles at most 255 slots, and slot 0-1 are always reserved for other use(e.g, slot 0 is for default binding when uploading/downloading data), hence slot 3~N will be automatically managed.

For Texture, limit N is get by `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS`.
For UBO, limit N is get by `GL_MAX_UNIFORM_BUFFER_BINDINGS`.

Binding Tex/Img/UBO can only be performed by `ProgState` or `ProgDraw`. They do not issue actual bind operation but only record the binding infomation. Actual bind will be performed when Drawing or Runing compute.   

### Bindless Texture and Image

Some devices support [bindless texture](https://www.khronos.org/opengl/wiki/Bindless_Texture), which can simplify the binding manager.

However, there are two extensions, [ARB_bindless_texture](https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_bindless_texture.txt) and [NV_bindless_texture](https://www.khronos.org/registry/OpenGL/extensions/NV/NV_bindless_texture.txt) have slightly different behaviour (ARB version is more strict and require change on GLSL). In order to conver the difference, OpenGLUtil provide some macro to give proper hint to the samplers/images in the default FBO.

`OGLU_TEX` and `OGLU_IMG` can be used to declare proper layout for sampler/image's uniform. While some uniforms will be manually assigned a index or other layout param, OpenGLUtil also provide `OGLU_TEX_LAYOUT` and `OGLU_IMG_LAYOUT` which only contains the param. They should be used as the last param, since they can be empty and you will have to deal with commas.

Note that **mixing bindless and bound resources are not supported, so all the texture/image in the default FBO should specicfy the same layout**. On platforms that does not support NV's extension but support ARB's extension, OpenGLUtil will defaultly uses bindless manager, whcih conflict glsl's defualt bound layout. 

### Program State

Drawcalls are expensive. After wrapping, it's more expensive since state needed to be rollbacked.

In order to reduce binding state changing, LRU cache is used. But uniform values are not handled in that way.

Each time you call oglProgram's `draw()`, an `ProgDraw` is created with current global state. ProgDraw will restore program state after it's deconstructed, so **avoid creating too many `ProgDraw`**.

Setting value to uniform / binding ubo or tex resources in `ProgDraw` is temporal, but setting them in `ProgState` or `oglProgram` is global.

`ProgDraw` will enforce current context to use corresponding glProgram, and assumes that state kept until it is destroyed. In rder to prevent mistakes, `ProgDraw` will cause a spinlock in the thread, so more than one `ProgDraw` at the same time will hang the app. It will also lock current FBO, so you will not be able to change FBO when `ProgDraw` has not been destructed.

### Multi-thread Worker

There is an optional worker, which can provide multi-thread operation ability. Operations inside are handled by AsyncExecutor, so only GL operations should be executed inside, leaving heavy task on other threads.

* Share Worker

  It has unique GL-context, but shared with object context(according to OpenGL's spec, [container objects](https://www.khronos.org/opengl/wiki/OpenGL_Object#Container_objects) like `FBO`,`ProgPipeline`,`TransFormFeedback`,`VAO`  are not shared).
  
  It's useful for uploading/downloading data

* Isolate Worker

  It has unique GL-context, not shared with object context.
  
  It can be used to do some extra work just like another GL program, and data can be sent back to main context by invoking main-thread or invoking Share Worker

## License

OpenGLUtil (including its component) is licensed under the [MIT license](../License.txt).
