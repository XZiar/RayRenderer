# Nailang

**Nailang(Nail), not an intresting language.**

---

Nailang is a simplified customizable language.

## Concept

### DataType

DataType is limited:

### Uint - 64bit unsigned integer
`123u`, `0x1ff`, `0b01010` are all `Uint`. 
### Int - 64bit signed integer
`123`, `-456` are all `Int`.
### FP - 64bit floating point
`123.`, `-456.`, `7e8`, `9.0e-3` are all `FP`. Note that exponential cannot be FP.
### Bool - boolean
`true`, `false`, `TruE` are all `Bool`. When calculated with numbers, `true`=`1` and `false`=`0`.
### Str - string
`"hello"`, `"\"world\""` are all `Str`. Escape character supported for `\r`,`\n`,`\0`,`\"`,`\t`,`\\`, others will simply keep later character.
### Custom - cunstom type
You can extend Nailang to support custom type. Custom type cannot be directly parsed, but can be lookuped and used by name. Meta-data can also be provided.

### Literal

Literal is parsed at parsing time, create an unnamed variable. It cannot be custom type.

### Variable

Variable is a memory unit to store data.

Name of variable currently limited on ASCII words, the leading character can be `ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_.`, and other characters can be `ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.`.

`.` is used to seperate variable name into parts. Buitin types does not support parts, but custom types may need this.

Specially, if `.` is the leading character, it means the variable should be at local.

**Variable only specify the name, the datatype is decided at runtime.**

### Function Call

Function call calls a function with arguments, may result an arguemnt.

`*Func(x,y)` is a function call, where `*` is the prefix for usage, `Func` is the name, `x` and `y` is the arguments.

### Expression

An expression is a combination of function calls. Operator is used for better readibility. Eg, `1+a`, `4 && false`, `!true`.

**Expresson can have one or two leave**, which means there's `UnaryExpr` and `BinaryExpr`.

**Operators have no precedence**, so you need to add a pair of parentheses manually. Eg, `(a*b)+c`, `(1+(2))`.

[Function call](#function-call), [variable](#variable), [literal](#literal) are all leaves of a expression. An expression can also be a leave of another expression, which make expression a tree.

An expression must be either **empty**(`()`), or **single element only**(`(x)`), or **leaef(ves) with an operator**(`1+x`).

### Command

A command is an executable unit. It can be a single expression or a block of expression.

For a single expression, it must ends with '`;`'.

### MetaFunction

Meta-function is used to provide extra information of a command, or to achieve control-flow effect.

Meta-function is indeed a function call with prefix'`@`'. It must be used standalone, so unlike commands, **it doesnot and cannot end with '`;`'**. Eg, `@If(x>1)`.

The return value of meta-function is discard. Runtime is required to evalute the function and modify the program states.

### Assign 

Assign is a command, means assign a value to a variable. The syntax is `(var) = (statement);`.

### FuncCall

FuncCall is a command, means calling a function and discard the result. The syntax is `$Func(x,y);`, a function call with prefix'`$`'. Runtime should be responsible for the side-effect. 

### Block

A block is a scope of commands, and is a basic unit for execution. The syntax is 
```
#Block.type("name")
{
    // content
}

#Block("name")
{
    // content
}
```
As shown above, type can be neglect, name should always be a str. A pair of curly brace is used to define the scope, and they are recommanded to be placed in newline.

### RawBlock

Raw block is a piece of content with unknown syntax. The syntax is
```
#Raw.type("name")
{guard
    // content
guard}
```
It's similar to [Block](#block), while guard string is required between the pair of curly brace.
This allow the content to contain any code with curly brace without knowning its syntax.

## Example

Here's some examples for the [gcd]() algorithm:

### gcd version 1

```
tmp = 1;
@While(tmp != 0)
#Block("")
{
    tmp = m % n;
    m = n;
    n = tmp;
}
```
is equivalent to C++ code below
```C++
uint64_t tmp = 1;
while (tmp != 0)
{
    tmp = m % n;
    m = n;
    n = tmp;
}
```

### gcd version 2

```
@While(true)
#Block("")
{
    tmp = m % n;
    m = n;
    n = tmp;
    @If(n==0)
    $Break();
}
```
is equivalent to C++ code below
```C++
while (true)
{
    const auto tmp = m % n;
    m = n;
    n = tmp;
    if (n == 0)
        break;
}
```

### gcd version 3

```
@DefFunc(m,n)
#Block("gcd")
{
    tmp = m % n;
    @If(tmp==0)
    $Return(n);

    $Return($gcd(n, tmp));
}
m = $gcd(m,n);
```
is equivalent to C++ code below
```C++
static constexpr auto refgcd(int64_t m, int64_t n)
{
    const uint64_t tmp = m % n;
    if (tmp == 0)
        return n;
    return refgcd(n, tmp);
}
m = refgcd(m, n);
```

## License

Nailang (including its component) is licensed under the [MIT license](../License.txt).