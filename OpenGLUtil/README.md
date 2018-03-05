# OpenGLUtil

A C++ wrapper of modern OpenGL.

It aims at providing a OOP wrapper which makes OpenGL's states transparent to upper programmers.

## Componoent

* **oglShader**

  OpenGL shader(loading/data storeage only)

* **oglBuffer**

  OpenGL buffer objects
  * oglBuffer -- any buffer object
  * oglTBO -- texture buffer object
  * oglUBO -- unifrom buffer object
  * oglEBO -- element buffer object(indexed element buffer)

* **oglTexture**
  OpenGL Texture

* **oglVAO**
  OpenGL vertex attribute object
  
  Draw calls are finally fired by oglVAO

* **oglProgram**
  OpenGL program
  
  It's like a "shader" in other engine, with resources slot binding with UBO or Texture, etc. 

* **oglUtil**
  OpenGL utility
  
  It providing environment initializing, debug message output and multi-thread worker

## Dependency

* [common](../common)
  * Wrapper -- an extended version of stl's shared_ptr
  * Exception -- an exception model with support for nested-exception, strong-type, unicode message, arbitray extra data 
  * StringEx -- some useful operation for string, including encoding-conversion

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

* [boost](http://www.boost.org/)
  * circular_buffer -- used for GL debug mesasge buffer

* C++17 required
  * optional -- used for some return value
  * tuple -- used for some internal structure

## Feature

### Shader

Despite of standard glsl shader like `*.vert`(Vertex Sahder), `*.frag`(Fragment Shader) and so on, OpenGLUtil also provid an extended shader format`*.glsl`.
It's main purpose is to merge multiple shader into one, which reduces redundent codes as well as eliminate bugs caused by careless.

Extended Shader is based on glsl's preprocessor.

A single line strted with `//@@$$` will be used to indicate what component this file should include(`VERT`,`FRAG`,`GEOM`...separated with `|` ).

For example, when compiling vertex shader, `OGLU_VERT` will be defined, so your codes can be compiled conditionally, while struct definition can be shared in any shader.

### Resource Management

Texture and UBO are binded to oglProgram(main shader), and their binding slot are limited.

OpenGLUtil include an LRU-policy resource manager to handle the bindings, which aims at least state-changing(binding texture is costy compared with uploading uniform(it may be buffered by driver)).

The manager handles at most 255 slots, and slot 0-3 are always reserved for other use(e.g, slot 0 is for default binding when uploading/dowloading data), hence slot 4~N will be automatically managed.

For Texture, limit N is get by `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS`.

For UBO, limit N is get by `GL_MAX_UNIFORM_BUFFER_BINDINGS`.

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
