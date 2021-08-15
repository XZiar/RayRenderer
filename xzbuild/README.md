# xzbuild
---
**xzbuild** is a simple build system, aiming to provide build ability in a declarative way.

## Restriction

* Currently only support *nix
* Currently rely in make as underlying build system
* Currently No chaching
* Not support specifying standalone flags for single file

## Requirement

* python 3.6+
* chardet (`python3 -m pip install chardet`)

## Commands

> Syntax help
```
python3 xzbuild help
```

> List all projects, or show the dependency of a specific project
```
python3 xzbuild list [<project>]
```

> Build projects
```
python3 xzbuild <command> <projects> [target] [platform] [/flags]
```
* `command` can be `<build|clean|buildall|cleanall|rebuild|rebuildall>`:
  * `build,clean,rebuild` only apply on the projects being specified. With `***all`, it will also apply on the dependencies of the projects.
  * `rebuild` is `clean`+`build`.
* `projects` is using syntax `<project,[project]|all>`:
  * `project` is simply the project name
  * multiple projects are concated by comma
  * `-project` is used to exclude some projects
  * `all` means all the projects being searched
* `target` can be `<Debug|Release>`, defaults to `Debug`
* `platform` can be `<x86|x64|ARM|ARM64>`, defaults to native platform
  * Some platform requires unique toolchain, so you may not be able to simply change it
* `flags` is extra flags that begins with `/`
  * `/verbose` show verbose info when build (show collected sources and command being executed)
  * `/threads=x` specify the threads for build, which act as `-jN`. Defaults to native cores
    * `x` can be number, so `/threads=3` specifies the exact thread count
    * `x` can begins with `x` and follows by a number, like `/threads=x1.5`, which specifies the thread count to be multiple of default value

## Usage

The idea is that multiple `projects` exist inside a `solution`, and they cna depend on each other.

`xzbuild.proj.json` in each project's folder to identify the project.

`xzbuild.sol.json` in solution's root folder to specify common info.

### project syntax

```json
{
    "name": str,
    "type": <"dynamic"|"static"|"executable">,
    "description": str,
    "dependency": xlist,
    "library": 
    {
        "static": xlist,
        "dynamic": xlist,
        ["path": xlist]
    },
    "targets":
    {
        "xxx":
        {
            "sources": xlist,
            ["defines": xlist],
            ["incpath": xlist],
            ["flags": xlist],
            ["pch": str]
        }
    }
}
```
#### **xlist**
xlist means a single item or a list of items.

* `str`: single item
* `[str,str]`: multiple items
* `{"cond": condvar, <"+"|"-">: <str|[]>}`: conditioned item
  
  `condvar` can be one or more `conditem`:
  * `str`: a single item
  * `[]`: a list of items, all the items need to satisfy the `cond`
  * `{}`: a dict, each key-value pair will be treated as an item, all the items need to satisfy the `cond`
  
  `cond` can be:
  * `ifhas`: check if conditem in `env`
  * `ifno`: check if conditem not in `env`
  * `ifeq`: use conditem's key to get value in `env`, check if is equal to conditem's value
  * `ifneq`: use conditem's key to get value in `env`, check if is not equal to conditem's value
  * `ifin`: use conditem's key to get value in `env`(defaults to empty list), check if it contains conditem's value
  * `ifnin`: use conditem's key to get value in `env`(defaults to empty list), check if it not contains conditem's value
  * `ifgt`: use conditem's key to get value in `env`(defaults to 0), check if is greater than conditem's value
  * `ifge`: use conditem's key to get value in `env`(defaults to 0), check if is greater than or equal to conditem's value
  * `iflt`: use conditem's key to get value in `env`(defaults to 0), check if is less than conditem's value
  * `ifle`: use conditem's key to get value in `env`(defaults to 0), check if is less than or equal to conditem's value

  If multiple `cond` exist, all of them need to be satisfied. If no `cond` exists, it's always satisfied.

  `<"+"|"-">` means add or del items. They are optional, but if both not exists, nothing will happend.

  Extra kv pair can exist to be used in post-process. e.g, `"dir": str` is being used to specify the parent directory of items.

### Dependency
Each project can depend on multiple other projects. Dependency need to be satisfied before next move.

### Library
Extra pre-built library dependency.

### Targets  
Targets represents different languages in the projects.

#### `cxx`
Pseudo target, provides common fileds like `flags`, `defines`, `incpath`, `dbgSymLevel`, `visibility`, `optimize`. It aims at `C/C++ compiler`.

* `dbgSymLevel: num`: controls debug symbol level like `-g0` or `-g3`. It can be override by build parameter `dsymlv`.

#### `c`
C target. Depends on `cxx`.

#### `cpp`
C++ target. Depends on `cxx`.

#### `asm`
ASM target that being compiled with C++ compiler.Depends on `cxx`.

#### `nasm`
NASM target that being compiled with [NASM](https://www.nasm.us/) compiler. Depends on `cxx`.

#### `ispc`
ISPC target that being compiled with [ISPC](https://ispc.github.io/) compiler.

#### `cuda`
CUDA target. `Experiemnetal`.

#### `rc`
Resouce target. `Experiemnetal`.

Use [self-written script](./ResourceCompiler.py) to embed extra resource binary decleared in `.rc` file.

## Feature

### [ResourceCompiler](./ResourceCompiler.py)

> On widnows, uses Visual Studio project to handle rc file

Much thanks to **[incbin](https://github.com/graphitemaster/incbin)** which provides the idea and usage of `incbin`. It's not being used due to the limitation of expose global symbol and incompatibility with linker when symbol is local.

ResourceCompiler utilize `GNU-style inline-assembly` and `.incbin` to embed resource, also relies on [`ResourceHelper`](../common/ResourceHelper.h) to provide common resource access interface.

Compare to relying on link-toolchian, it better handles naming issue and Darwin platform (which requires to embed resource at final link stage).

There's [special code path](./RCInclude.h) for different platform and architechture to retieve the address of resource. 

| Platform | Architecture |
|:-------|:-------:|
| Ubuntu (Linux) | x86 / ARM(untested) |
| Android (Linux) | x86(untested) / ARM |
| iOS (Darwin) | ARM |
| macOS (Darwin) | ARM (untested) |

## Roadmap

* Remove use of make:
  * Add parsing of `.d` file
  * Record and cache dependency info
  * Execute build command inside xzbuild
  * Add multi-thread support
* Add intra-project parallism
* Add pseudo project, invokes package-manager to install extra dependency, or download 3rdparty source.

## License

xzbuild (including its component) is licensed under the [MIT license](../License.txt).