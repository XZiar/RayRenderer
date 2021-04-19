import abc
import glob
import inspect
import os

from . import COLOR, writeItems, writeItem
from . import PowerList as PList
from .Environment import findAppInPath


class BuildTarget(metaclass=abc.ABCMeta):
    @staticmethod
    @abc.abstractmethod
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

    @staticmethod
    def preparePaths(proj, env:dict, paths:list):
        solDir = env["rootDir"]
        buildDir = os.path.join(solDir, proj.buildPath)
        return [path.replace("$(SolutionDir)", solDir).replace("$(BuildDir)", buildDir) for path in paths]
    
    def write(self, file):
        file.write(f"\n\n# For target [{self.prefix()}]\n")
        writeItems(file, self.prefix()+"_srcs", self.sources)
        writeItems(file, self.prefix()+"_flags", self.flags)

    def printSources(self):
        print(f'{COLOR.magenta}[{self.prefix()}] Sources:\n{COLOR.White(" ".join(self.sources))}')

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
        a,d = PList.solveElementList(target, "sources", env, adddir)
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
        a,d = PList.solveElementList(target, "flags", env)
        self.flags = PList.combineElements(self.flags, a, d)

    def finishTarget(self, proj, env:dict):
        '''finish solving target with project info'''
        pass

    def __repr__(self):
        return str(vars(self))


class CXXTarget(BuildTarget, metaclass=abc.ABCMeta):
    @staticmethod
    @abc.abstractmethod
    def langVersion() -> str:
        pass

    def __init__(self, targets, env:dict):
        self.defines = []
        self.incpath = []
        self.pch = ""
        self.debugLevel = "-g3"
        self.optimize = ""
        self.version = ""
        self.visibility = "hidden"
        super().__init__(targets, env)

    def solveTarget(self, targets, env:dict):
        self.flags += ["-Wall", "-pedantic", "-pthread", "-Wno-unknown-pragmas", "-Wno-ignored-attributes", "-Wno-unused-local-typedefs", "-fno-common"]
        self.flags += [env["archparam"]]
        self.flags += ["-m64" if env["platform"] == "x64" else "-m32"]
        self.optimize = "-O2" if env["target"] == "Release" else "-O0"
        if env["compiler"] == "clang":
            self.flags += ["-Wno-newline-eof"]
        if env["target"] == "Release":
            self.defines += ["NDEBUG"]
            if env["paras"].get("lto", "on") == "on":
                self.flags += ["-flto"]
        cxx = targets.get("cxx")
        if cxx is not None:
            self.visibility = cxx.get("visibility", self.visibility)
            a,_ = PList.solveElementList(cxx, "debug", env)
            self.debugLevel = a[0] if len(a)>0 else self.debugLevel
            a,_ = PList.solveElementList(cxx, "optimize", env)
            self.optimize = a[0] if len(a)>0 else self.optimize
            a,d = PList.solveElementList(cxx, "flags", env)
            self.flags = PList.combineElements(self.flags, a, d)
            a,d = PList.solveElementList(cxx, "defines", env)
            self.defines = PList.combineElements(self.defines, a, d)
            a,d = PList.solveElementList(cxx, "incpath", env)
            self.incpath = PList.combineElements(self.incpath, a, d)
        super().solveTarget(targets, env)
        target = targets[self.prefix()]
        self.pch = target.get("pch", "")
        self.version = target.get("version", self.langVersion())
        self.visibility = target.get("visibility", self.visibility)
        a,_ = PList.solveElementList(target, "debug", env)
        self.debugLevel = a[0] if len(a)>0 else self.debugLevel
        a,_ = PList.solveElementList(target, "optimize", env)
        self.optimize = a[0] if len(a)>0 else self.optimize
        a,d = PList.solveElementList(target, "defines", env)
        self.defines = PList.combineElements(self.defines, a, d)
        a,d = PList.solveElementList(target, "incpath", env)
        self.incpath = PList.combineElements(self.incpath, a, d)
        self.flags += [f"-fvisibility={self.visibility}"]

    def finishTarget(self, proj, env:dict):
        self.incpath = BuildTarget.preparePaths(proj, env, self.incpath)

    def write(self, file):
        super().write(file)
        writeItems(file, self.prefix()+"_defs", self.defines)
        writeItems(file, self.prefix()+"_incpaths", self.incpath)
        writeItems(file, self.prefix()+"_flags", [self.version, self.debugLevel, self.optimize], state="+")
        writeItem (file, self.prefix()+"_pch", self.pch)


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
        a,d = PList.solveElementList(target, "incpath", env)
        self.incpath = list(set(a) - set(d))

    def finishTarget(self, proj, env:dict):
        self.incpath = BuildTarget.preparePaths(proj, env, self.incpath)

    def write(self, file):
        super().write(file)
        writeItems(file, self.prefix()+"_incpaths", self.incpath)


class RCTarget(BuildTarget):
    @staticmethod
    def prefix() -> str:
        return "rc"


class ISPCTarget(BuildTarget):
    compiler = None
    @staticmethod
    def prefix() -> str:
        return "ispc"
    @staticmethod
    def initEnv(env: dict):
        if "ISPCCOMPILER" not in os.environ:
            paths = findAppInPath("ispc")
            if len(paths) > 0:
                ISPCTarget.compiler = os.path.join(paths[0], "ispc")
            else:
                print(COLOR.Yellow("Seems ISPC is not found"))


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
        if ISPCTarget.compiler:
            writeItem(file, "ISPCCOMPILER", ISPCTarget.compiler, state="?")
            pass


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
            print(COLOR.Yellow("Latest CUDA does not support 32bit any more"))

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
            print(COLOR.Yellow("CUDA Home directory not found"))

    def solveTarget(self, targets, env:dict):
        self.flags += ["-pg", "-lineinfo", "-use_fast_math", "-res-usage", "--source-in-ptx"]
        self.flags += ["-m64" if env["platform"] == "x64" else "-m32"]
        self.optimize = "-O2" if env["target"] == "Release" else "-O0"
        if env["target"] == "Release":
            self.defines += ["NDEBUG"]
        cuda = targets.get("cuda")
        if cuda is not None:
            a,_ = PList.solveElementList(cuda, "hostDebug", env)
            self.hostDebug = a[0] if len(a)>0 else self.hostDebug
            a,_ = PList.solveElementList(cuda, "deviceDebug", env)
            self.deviceDebug = a[0] if len(a)>0 else self.deviceDebug
            a,_ = PList.solveElementList(cuda, "optimize", env)
            self.optimize = a[0] if len(a)>0 else self.optimize
            a,d = PList.solveElementList(cuda, "flags", env)
            self.flags = PList.combineElements(self.flags, a, d)
            a,d = PList.solveElementList(cuda, "defines", env)
            self.defines = PList.combineElements(self.defines, a, d)
            a,d = PList.solveElementList(cuda, "incpath", env)
            self.incpath = PList.combineElements(self.incpath, a, d)
            a,d = PList.solveElementList(cuda, "arch", env)
            self.arch = PList.combineElements(self.arch, a, d)
            self.version = cuda.get("version", "-std=c++14")
        super().solveTarget(targets, env)

    def finishTarget(self, proj, env:dict):
        self.incpath = BuildTarget.preparePaths(proj, env, self.incpath)

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