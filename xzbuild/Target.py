import abc
import glob
import inspect
import json
import os
import sys
import time
from collections import OrderedDict
from ._Rely import *
from .Environment import findAppInPath


class BuildTarget(metaclass=abc.ABCMeta):
    @abc.abstractstaticmethod
    def prefix() -> str:
        pass
    @staticmethod
    def initEnv(env: dict):
        '''initialize environment'''
        pass
    @staticmethod
    def modifyProject(proj, env:dict):
        '''modify parent project'''
        pass
    
    def write(self, file):
        file.write("\n\n# For target [{}]\n".format(self.prefix()))
        writeItems(file, self.prefix()+"_srcs", self.sources)
        writeItems(file, self.prefix()+"_flags", self.flags)

    def printSources(self):
        print("{clr.magenta}[{}] Sources:\n{clr.white}{}{clr.clear}".format(self.prefix(), " ".join(self.sources), clr=COLOR))

    def __init__(self, targets, env:dict):
        self.sources = []
        self.flags = []
        self.solveTarget(targets, env)

    def solveSource(self, targets, env:dict):
        def adddir(ret:tuple, ele:dict, env:dict):
            if "dir" in ele:
                dir = ele["dir"]
                return tuple(list(os.path.join(dir, i) for i in x) for x in ret)
            return ret
        target = targets[self.prefix()]
        a,d = solveElementList(target, "sources", env, adddir)
        adds = set(f for i in a for f in glob.glob(i))
        dels = set(f for i in d for f in glob.glob(i))
        forceadd = adds & set(a)
        forcedel = dels & set(d)
        self.sources = list(((adds - dels) | forceadd) - forcedel)
        self.sources.sort()

    def solveTarget(self, targets, env:dict):
        '''solve sources and flags'''
        self.solveSource(targets, env)
        target = targets[self.prefix()]
        a,d = solveElementList(target, "flags", env)
        self.flags = combineElements(self.flags, a, d)

    def __repr__(self):
        return str(vars(self))

class CXXTarget(BuildTarget, metaclass=abc.ABCMeta):
    @abc.abstractstaticmethod
    def langVersion() -> str:
        pass

    def __init__(self, targets, env:dict):
        self.defines = []
        self.incpath = []
        self.pch = ""
        self.debugLevel = "-g3"
        self.optimize = ""
        self.version = ""
        super().__init__(targets, env)

    def solveTarget(self, targets, env:dict):
        self.flags += ["-Wall", "-pedantic", "-march=native", "-pthread", "-Wno-unknown-pragmas", "-Wno-ignored-attributes", "-Wno-unused-local-typedefs"]
        self.flags += ["-m64" if env["platform"] == "x64" else "-m32"]
        self.optimize = "-O2" if env["target"] == "Release" else "-O0"
        if env["compiler"] == "clang":
            self.flags += ["-Wno-newline-eof"]
        if env["target"] == "Release":
            self.defines += ["NDEBUG"]
            self.flags += ["-flto"]
        cxx = targets.get("cxx")
        if cxx is not None:
            a,_ = solveElementList(cxx, "debug", env)
            self.debugLevel = a[0] if len(a)>0 else self.debugLevel
            a,_ = solveElementList(cxx, "optimize", env)
            self.optimize = a[0] if len(a)>0 else self.optimize
            a,d = solveElementList(cxx, "flags", env)
            self.flags = combineElements(self.flags, a, d)
            a,d = solveElementList(cxx, "defines", env)
            self.defines = combineElements(self.defines, a, d)
            a,d = solveElementList(cxx, "incpath", env)
            self.incpath = combineElements(self.incpath, a, d)
        super().solveTarget(targets, env)
        target = targets[self.prefix()]
        self.pch = target.get("pch", "")
        self.version = target.get("version", self.langVersion())
        a,_ = solveElementList(target, "debug", env)
        self.debugLevel = a[0] if len(a)>0 else self.debugLevel
        a,_ = solveElementList(target, "optimize", env)
        self.optimize = a[0] if len(a)>0 else self.optimize
        a,d = solveElementList(target, "defines", env)
        self.defines = combineElements(self.defines, a, d)
        a,d = solveElementList(target, "incpath", env)
        self.incpath = combineElements(self.incpath, a, d)

    def write(self, file):
        super().write(file)
        writeItems(file, self.prefix()+"_defs", self.defines)
        writeItems(file, self.prefix()+"_incpaths", self.incpath)
        writeItems(file, self.prefix()+"_flags", [self.version, self.debugLevel, self.optimize], state="+")
        writeItem(file, self.prefix()+"_pch", self.pch)

class CPPTarget(CXXTarget):
    @staticmethod
    def prefix() -> str:
        return "cpp"
    @staticmethod
    def langVersion() -> str:
        return "-std=c++17"

class CTarget(CXXTarget):
    @staticmethod
    def prefix() -> str:
        return "c"
    @staticmethod
    def langVersion() -> str:
        return "-std=c11"

class ASMTarget(CXXTarget):
    @staticmethod
    def prefix() -> str:
        return "asm"
    @staticmethod
    def langVersion() -> str:
        return ""

class NASMTarget(BuildTarget):
    @staticmethod
    def prefix() -> str:
        return "nasm"
    def __init__(self, targets, env:dict):
        self.incpath = []
        super().__init__(targets, env)

    def solveTarget(self, targets, env:dict):
        self.flags = ["-g", "-DELF"]
        if env["platform"] == "x64":
            self.flags += ["-f elf64", "-D__x86_64__"]
        else:
            self.flags += ["-f elf32"]
        super().solveTarget(targets, env)
        target = targets["nasm"]
        a,d = solveElementList(target, "incpath", env)
        self.incpath = list(set(a) - set(d))
    def write(self, file):
        super().write(file)
        writeItems(file, self.prefix()+"_incpaths", self.incpath)

class RCTarget(BuildTarget):
    @staticmethod
    def prefix() -> str:
        return "rc"

class ISPCTarget(BuildTarget):
    @staticmethod
    def prefix() -> str:
        return "ispc"
    def __init__(self, targets, env:dict):
        self.targets = ["sse4", "avx2"]
        super().__init__(targets, env)

    def solveTarget(self, targets, env:dict):
        self.flags = ["-g", "-O2", "--opt=fast-math", "--pic"]
        if env["platform"] == "x64":
            self.flags += ["--arch=x86-64"]
        else:
            self.flags += ["--arch=x86"]
        super().solveTarget(targets, env)

    def write(self, file):
        super().write(file)
        writeItems(file, self.prefix()+"_targets", self.targets)
        writeItem(file, self.prefix()+"_flags", "--target="+(",".join(self.targets)), state="+")


class CUDATarget(BuildTarget):
    cudaHome = None
    @staticmethod
    def prefix() -> str:
        return "cuda"
    @staticmethod
    def initEnv(env: dict):
        CUDATarget.cudaHome = os.environ.get("CUDA_PATH")
        if CUDATarget.cudaHome == None:
            paths = findAppInPath("nvcc")
            if len(paths) > 0:
                CUDATarget.cudaHome = os.path.abspath(os.path.join(paths[0], os.pardir))
        if env["platform"] == "x86":
            print("{clr.yellow}Latest CUDA does not support 32bit any more{clr.clear}".format(clr=COLOR))

    @staticmethod
    def modifyProject(proj, env:dict):
        proj.libDynamic += ["cuda", "cudart"]
        if CUDATarget.cudaHome:
            proj.libDirs += [os.path.join(CUDATarget.cudaHome, "lib64")]

    def __init__(self, targets, env:dict):
        self.defines = []
        self.incpath = []
        self.hostDebug = "-g"
        self.deviceDebug = ""
        self.optimize = ""
        self.version = ""
        self.arch = []
        super().__init__(targets, env)
        if CUDATarget.cudaHome:
            self.incpath += [os.path.join(CUDATarget.cudaHome, "include")]
        else:
            print("{clr.yellow}CUDA Home directory not found{clr.clear}".format(clr=COLOR))

    def solveTarget(self, targets, env:dict):
        self.flags += ["-pg", "-lineinfo", "-use_fast_math", "-res-usage", "--source-in-ptx"]
        self.flags += ["-m64" if env["platform"] == "x64" else "-m32"]
        self.optimize = "-O2" if env["target"] == "Release" else "-O0"
        if env["target"] == "Release":
            self.defines += ["NDEBUG"]
        cuda = targets.get("cuda")
        if cuda is not None:
            a,_ = solveElementList(cuda, "hostDebug", env)
            self.hostDebug = a[0] if len(a)>0 else self.hostDebug
            a,_ = solveElementList(cuda, "deviceDebug", env)
            self.deviceDebug = a[0] if len(a)>0 else self.deviceDebug
            a,_ = solveElementList(cuda, "optimize", env)
            self.optimize = a[0] if len(a)>0 else self.optimize
            a,d = solveElementList(cuda, "flags", env)
            self.flags = combineElements(self.flags, a, d)
            a,d = solveElementList(cuda, "defines", env)
            self.defines = combineElements(self.defines, a, d)
            a,d = solveElementList(cuda, "incpath", env)
            self.incpath = combineElements(self.incpath, a, d)
            a,d = solveElementList(cuda, "arch", env)
            self.arch = combineElements(self.arch, a, d)
            self.version = cuda.get("version", "-std=c++14")
        super().solveTarget(targets, env)

    def write(self, file):
        super().write(file)
        writeItems(file, self.prefix()+"_defs", self.defines)
        writeItems(file, self.prefix()+"_incpaths", self.incpath)
        writeItems(file, self.prefix()+"_flags", [self.version, self.hostDebug, self.deviceDebug, self.optimize], state="+")
        if len(self.arch) > 0:
            writeItem(file, self.prefix()+"_flags", "-arch="+",".join(self.arch), state="+")




def _getSubclasses(clz):
    for c in clz.__subclasses__():
        if inspect.isabstract(c):
            for subc in _getSubclasses(c):
                yield subc
        else:
            yield c

_AllTargets = list(_getSubclasses(BuildTarget))