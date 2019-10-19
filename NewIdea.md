# FancyCL

## Target
C++书写，生成OpenCL代码。

* template
* type-safe，compile time type check
* full-feature support

## Design
声明类，继承于Kernel类（主体功能支持），可选继承于Templated类（要求提供参数以实例化，通过宏声明名称映射）。

书写代码于operator()，代码逻辑将完整包留。

构造类实例需要提供模板参数，实例执行后返回OpenCL Kernel元数据，可用于导出源代码。

元数据包含：

* 外部编译期常量，如模板参数，导出为宏
* struct定义，导出为struct
* 外部宏定义（受限），导出为宏
* 编译期数据，导出为constant数据
* 内部函数，导出为单独函数

执行前generator植入context到threadlocal用于登记操作。generator调用operator()，内部逻辑执行过程中登记各项操作。

函数体内所有普通变量都将在执行时被计算，其结果成为常量。若要输出变量声明，需要单独声明专属变量类型。

if等语句表达式将在执行时被计算，代码逻辑将部分输出。通过内部提供的if_等语句可以保留整体逻辑。

### Variable 专属变量类型
所有数据类型都需要特化自`Variable<Type>`，`Type`用于类型匹配，`Variable`用于参数检查。

Variable不可变，不可move/copy，

