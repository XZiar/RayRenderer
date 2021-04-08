# Nailang

![LOGO](./Nailang.ico)

**Nailang(Nail), not an intresting language.**

---

Nailang is a simplified customizable language.

## Concept

### Literal
Literal is parsed at parsing time.

#### Uint - 64bit unsigned integer
`123u`, `0x1ff`, `0b01010` are all `Uint`. 
#### Int - 64bit signed integer
`123`, `-456` are all `Int`.
#### FP - 64bit floating point
`123.`, `-456.`, `7e8`, `9.0e-3` are all `FP`. Note that exponential cannot be FP.
#### Bool - boolean
`true`, `false`, `TruE` are all `Bool`. When calculated with numbers, `true`=`1` and `false`=`0`.
#### Str - string
`"hello"`, `"\"world\""` are all `Str`. Escape character supported for `\r`,`\n`,`\0`,`\"`,`\t`,`\\`, others will simply keep later character.

### Variable

Variable is a memory unit to store data.

Name of variable currently limited on ASCII words, the leading character can be ``:`ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_``, and other characters can be `ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_`.

`:` and `` ` `` is the special leading character. `:` means the variable should be at local and `` ` `` means the variable should be at root.

**Variable only specify the name, the datatype is decided at runtime.**

### Function Call

Function call calls a function with arguments, may result an arguemnt.

`*Func(x,y)` is a function call, where `*` is the prefix for usage, `Func` is the name, `x` and `y` is the arguments.

### Query

There're 2 kinds of query:

* `Subfield`: Access to a field of an object. Eg, `a.b.c`.
* `Indexer`: just like a single arg function that access some content using an index. Eg, `a[12]["c"]`.

Note that queries will be chained and stored compactly, and runtime may be able to consume multiple queries at a time.

### Expression

An expression is in fact an AST node, can be literal, function call, or an operator-expression.

### Operator

Operator is used for better readibility, eg, `1+a`, `4 && false`, `!true`.

**Operators have no precedence**, so you need to add a pair of parentheses manually. Eg, `(a*b)+c`, `(1+(2))`.

**Operator Expression can have one, two, three leaves**, which means there's `Unary Operator`, `Binary Operator`, `Ternary Operator`.

| Operator | Name | Type | Argument | Return | Example |
|:--------:|:----:|:----:|:--------:|:------:|--------:|
| `==` | Equal         |  Binary | Num, Bool, Str | Bool     |`1==3`|
| `!=` | Not Equal     |  Binary | Num, Bool, Str | Bool     |`1!=3`|
| `<`  | Less Than     |  Binary | Num            | Bool     |`1<3`|
| `<=` | Less Equal    |  Binary | Num            | Bool     |`1<=3`|
| `>`  | Greater Than  |  Binary | Num            | Bool     |`1>3`|
| `>=` | Greater Equal |  Binary | Num            | Bool     |`1>=3`|
| `&&` | And           |  Binary | Boolable       | Bool     |`true && 3`|
|`\|\|`| Or            |  Binary | Boolable       | Bool     |`1 \|\| false`|
| `+`  | Add           |  Binary | Num, Str       | Num, Str |`1 + 3`|
| `-`  | Minus         |  Binary | Num            | Num      |`1 - 3`|
| `*`  | Multiply      |  Binary | Num            | Num      |`1 * 3`|
| `/`  | Division      |  Binary | Num            | Num      |`1 / 3`|
| `%`  | Reminder      |  Binary | Num            | Num      |`1 % 3`|
| `??` | Value Or      |  Binary | Any            | Any      |`a ?? 3`|
| `?`  | Check Exists  |   Unary | Var            | Bool     |`?abc`|
| `!`  | Not           |   Unary | Boolable       | Bool     |`!true`|
|`? :` | Conditional   | Ternary | Any            | Any      |`a ? 1 : 3`|

#### Short-circuit evaluation

**`&&`, `||`, `??`, `? :` support short-circuit evaluation**, but it's garuenteed by NailangExecutor, whose behavior can be override.

**`??`, `?` operator designed for testing if a varaible exists**, so they only accept LateBindVar as first operand. 

### Statement

A statement is an executable unit, which must ends with '`;`'. It can also be a block of expression, or a rawblock that reserved for special purpose.

### MetaFunction

Meta-function is used to provide extra information of a statement, or to achieve control-flow effect.

Meta-function is indeed a function call with prefix'`@`'. It must be used standalone, so unlike statement, **it doesnot and cannot end with '`;`'**. Eg, `@If(x>1)`.

The return value of meta-function is discard. Executor is required to evalute the function and modify the program states.

### AssignExpr

AssignExpr is a statement, means assign a value to a variable. The syntax is `(var) (op) (statement);`.

The `op` is usually `=`, but self-modifying operator is also introduced for better readibility or saving duplicated variable-lookup.

| op | Example | Actual | Var==Null | Var!=Null|
|:--:|:--------|:-------|:---------:|:--------:|
|`=` |`a = 1`  |`a = 1`     | Pass  | Pass  |
|`+=`|`a += 1` |`a = a + 1` | Throw | Pass  |
|`-=`|`a -= 1` |`a = a - 1` | Throw | Pass  |
|`/=`|`a /= 1` |`a = a / 1` | Throw | Pass  |
|`%=`|`a %= 1` |`a = a % 1` | Throw | Pass  |
|`?=`|`a ?= 1` |`a = 1`     | Pass  | Skip  |
|`:=`|`a := 1` |`a = 1`     | Pass  | Throw |

`?=` is a special assignment whose calculation and assignment happens only when variable is empty.

`:=` is a special assignment that force the target variable does not exists.

### FuncCall

FuncCall is a statement, means calling a function and discard the result. The syntax is `$Func(x,y);`, a function call with prefix'`$`'. Runtime should be responsible for the side-effect. 

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
:tmp = 1u;
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
    :tmp = m % n;
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
static constexpr auto refgcd(uint64_t m, uint64_t n)
{
    const auto tmp = m % n;
    if (tmp == 0)
        return n;
    return refgcd(n, tmp);
}
m = refgcd(m, n);
```

## Execute

Nailang is aimed to be a extensible script language. Due to the demand of extensions, static type check is almost unavaliable so everything is dynamic.

After parsing, Nailang source code will be converted into AST-like format and execution performed at AST level.

### Memory Management

#### MemPool

In order to avoid memory fragments, `MemPool` is introduced. 

It is a trunk-based storage with no de-allocation support. All AST info are stored compactly inside `MemPool`.

#### EvaluateContext

EvaluateContext is to store runtime information, including variables and local functions.

### `Expr` and `Arg`

At AST level, literals and variables are stored inside `Expr`. But actual function will accept `Arg`, so there will be a conversion.

* `Expr` support `LateBindVar`, which is simply the name of the variable. After evaluation, it will become an `Arg` or proxy to an `Arg`. 

* `Expr` is constant, so the name of variable will be stored at source itself, the string content will be stored at [MemPool](#mempool) if needed. But `Arg` is dynamic, so its content will be stored at heap individually.

`Arg` can store the literal type, or some runtime type:
#### Array - array-ref

Stores the reference to an array. (Does not hold the ownership).

#### Custom - custom type

You can extend Nailang with custom type with the metadata being provided.

### Type Promotion

Type promotion is similar to C++ since the implementation is C++ based.

## License

Nailang (including its component) is licensed under the [MIT license](../License.txt).