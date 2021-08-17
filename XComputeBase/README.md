# XComputeBase

Shared utility and basic framework for XrossCompute.

It provides utilities for different compute platforms, eg, OpenCL, OpenGL, DirectX.

## Component

### **XCompDebug**

Platform-independent debugging framework.

* **ArgsLayout** 
  
  Layout of args for a message. (offset, length, datatype, name)

* **MessageBlock** 
  
  Metadata for a message. (formatter, layout, name)

* **WorkItemInfo** 
  
  Metadata for a single work item. It can be extended by specific platform.

* **InfoProvider** 
  
  The provider to retrive `WorkItemInfo` from debug data.

* **DebugManager** 
  
  The manager to hold both `InfoProvider` and different `MessageBlock`s. There can be at most 250 different Messages being defined.

* **DebugPackage** 
  
  Portable debug data package. Info and data are hold using `AlignedBuffer`, which allows external allocator.

* **CachedDebugPackage** 
  
  Debug data package with automatic string cache. The text of message will be cached at the first time being accessed.

* **XCNLDebugExt** 
  
  Allow custom XCNLExtension to provide debugging support, handles message define parsing.

#### Design

Each program can generate pre-defined debug messages. The message defines will contain `name`(use as UID), `formatter`, `arg datatype`s, and optionally `arg name`s.

Syntax for arg info: `[name:]datatype`, eg, `i32v4`, `Tmp:f16`.

The platform-specific extension is responsible for generating native code to produce th message. The layout of args is stored inside `ArgsLayout`.

### **XCompNailang**

Basic framework using [nailang](../Nailang).

XrossCompute uses Nailang as its runtime and source format.

`Block`s are executable code to perform configuration. `RawBlock`s are the actual native code being output.

There are several types of `RawBlock`:
* Global
  
  native code that always being output.

* Struct
  
  defines of data structure. Common Syntax for the structure is not ready.

* Instance
  
  A basic runnable kernel. Some platforms(OpenCL) allows multiple kernels in a file, while some(DX/GL) does not. So it's platform-specific runtime's responsibility to correctly generate output file with kernels.

* Template
  
  A basic block that can be reused multiple times. Similar to a macro that expended within XCNL runtime.

#### Native-XCNL interact

To allow interaction with XCNL in native code, XCNL uses Nailang's `ReplaceEngine` to do pattern matching.

* **replace-block**
  
  A replace block is similar to a `#if-#endif` block. It accept a single Nailang statement to determine if the content will be output.

  Syntax:
  ```
  $$@{True}
  will be output
  @$$
  $$@{3 == 2}
  will be ignore
  @$$
  ```

* **replace-variable**

  A replace variable is simiar to a macro. It accept a `LateBindVar` so that XCNL can query from its context and output corresponding content.

  Additionally, there's fastpath to get native VecTypeName:
  * `@xxx` will treat `xxx` as VecTypeStr.
  * `#xxx` will treat `xxx` as LateBindVar, whose str content is VecTypeStr.

  Syntax:
  ```
  const int flag = $$!{`IsDebug};
  const $$!{@i32v4}   data1;
  const $$!{#OutType} output;
  ```

* **replace-function**

  A replace function is simiar to a macro function. It accept a function name so that XCNL can generate corresponding function calls.

  The args of replace function can only be native code. So if you want to use XCNL variable inside it, use `replace-variable` since it will be replaced first.

  Syntax:
  ```
  const int flag = $$!foo(bar);
  const int data = $$!foo($$!{`IsDebug});
  ```

#### XCNL Extension

To make XCNL Runtime more extensible, XCNL support extensions.

* **BeginXCNL**

  Perform initialization.

* **ConfigFunc**

  When XCNL code calls a unsolved function.
  
* **ReplaceFunc**

  When there's an unsolved replace-function.
  
* **BeginInstance**

  Perform initialization for an instance. 
  
  Due to serialize execution, extension can hold reference of the instance context to allow specific processing with different instances.
  
* **InstanceMeta**

  When there's an unsolved meta-function for the Instance.
  
* **FinishInstance**

  Perform finalization for an instance. 

  Allow extension to inject content here. Should also release the reference to the instance here.
  
* **FinishXCNL**

  Perform finalization.

#### InstanceContext

There's basic support for metadata for an instance: `BodyPrefixes`, `BodySuffixes`.

Platform-specific runtime can extend it with more info. 

## Dependency

* [Nailang](../Nailang)

  XCNL language support

* [SystemCommon](../SystemCommon)
  
  OS/Hardware related support, and logging support

## License

XComputeBase (including its component) is licensed under the [MIT license](../License.txt).
