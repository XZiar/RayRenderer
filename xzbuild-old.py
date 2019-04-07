import abc
import glob
import inspect
import json
import os
import platform
import re
import subprocess
import sys
import time
from collections import deque

class COLOR:
    black	= "\033[90m"
    red		= "\033[91m"
    green	= "\033[92m"
    yellow	= "\033[93m"
    clue	= "\033[94m"
    magenta	= "\033[95m"
    cyan	= "\033[96m"
    white	= "\033[97m"
    clear	= "\033[39m"

solDir = os.getcwd()

def collectEnv() -> dict:
    intrinMap = \
    {
        "__SSE__":    "sse",
        "__SSE2__":   "sse2",
        "__SSE3__":   "sse3",
        "__SSSE3__":  "ssse3",
        "__SSE4_1__": "sse41",
        "__SSE4_2__": "sse42",
        "__AVX__":    "avx",
        "__FMA__":    "fma",
        "__AVX2__":   "avx2",
        "__AES__":    "aes",
        "__SHA__":    "sha",
        "__PCLMUL__": "pclmul",
    }
    env = {"rootDir": solDir, "target": "Debug"}
    is64Bits = sys.maxsize > 2**32
    env["platform"] = "x64" if is64Bits else "x86"
    env["incDirs"] = []
    env["incDirs"] += [x+"/include" for x in [os.environ.get("CPP_DEPENDENCY_PATH")] if x is not None]
    cppcompiler = os.environ.get("CPPCOMPILER", "g++")
    defs = []
    osname = platform.system()
    if not osname == "Windows":
        rawdefs = subprocess.check_output("{} -march=native -dM -E - < /dev/null".format(cppcompiler), shell=True)
        defs = set([d.split()[1] for d in rawdefs.decode().splitlines()])
    env["intrin"] = set(i[1] for i in intrinMap.items() if i[0] in defs)
    env["compiler"] = "clang" if "__clang__" in defs else "gcc"
    return env
def writeEnv(env:dict):
    os.makedirs("./" + env["objpath"], exist_ok=True)
    with open(os.path.join(env["objpath"], "xzbuild.sol.mk"), 'w') as file:
        file.write("# xzbuild per solution file\n")
        file.write("# written at [{}]\n".format(time.asctime(time.localtime(time.time()))))
        for k,v in env.items():
            if isinstance(v, str):
                file.write("xz_{}\t = {}\n".format(k, v))
        file.write("xz_incDir\t = {}\n".format(" ".join(env["incDirs"])))


def solveElementList(target, field:str, env:dict, postproc=None) -> tuple:
    tests = \
    {
        "ifhas": lambda x: x in env,
        "ifno" : lambda x: x not in env,
        "ifeq" : lambda x: env.get(x[0]) == x[1],
        "ifneq": lambda x: env.get(x[0]) != x[1],
        "ifin" : lambda x: x[1] in env.get(x[0], []),
        "ifnin": lambda x: x[1] not in env.get(x[0], []),
    }
    def checkMatch(obj:dict, name:str, test) -> bool:
        target = obj.get(name)
        if target is None:
            return True
        if isinstance(target, list):
            return all([test(t) for t in target])
        if isinstance(target, dict):
            return all([test(t) for t in target.items()])
        else:
            return test(target)
    def solveElement(element, env:dict, postproc) -> tuple:
        if isinstance(element, list):
            return (element, [])
        if not isinstance(element, dict):
            return ([element], [])
        if not all(checkMatch(element, n, t) for n,t in tests.items()):
            return ([], [])
        adds = element.get("+", [])
        adds = adds if isinstance(adds, list) else [adds]
        dels = element.get("-", [])
        dels = dels if isinstance(dels, list) else [dels]
        ret = (adds, dels)
        if not postproc == None:
            ret = postproc(ret, element, env)
        return ret
    eles = target.get(field, [])
    if not isinstance(eles, list):
        return solveElement(eles, env, postproc)
    middle = list(solveElement(ele, env, postproc) for ele in eles)
    adds = list(i for a,_ in middle for i in a)
    dels = list(i for _,d in middle for i in d)
    return (adds, dels)

def getSubclasses(clz):
    for c in clz.__subclasses__():
        if inspect.isabstract(c):
            for subc in getSubclasses(c):
                yield subc
        else:
            yield c

class Project:
    @staticmethod
    def _writeItems(file, name:str, val:list, state = ":"):
        file.write("{}\t{}= {}\n".format(name, state, " ".join(val)))
    @staticmethod
    def _writeItem(file, name:str, val:str, state = ":"):
        file.write("{}\t{}= {}\n".format(name, state, val))
    class BuildTarget(metaclass=abc.ABCMeta):
        @abc.abstractstaticmethod
        def prefix() -> str:
            pass
        
        def write(self, file):
            file.write("\n\n# For target [{}]\n".format(self.prefix()))
            Project._writeItems(file, self.prefix()+"_srcs", self.sources)
            Project._writeItems(file, self.prefix()+"_flags", self.flags)

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
        def solveTarget(self, targets, env:dict):
            self.solveSource(targets, env)
            target = targets[self.prefix()]
            a,d = solveElementList(target, "flags", env)
            self.flags = list(set(a + self.flags) - set(d))
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
            self.optimize = "-O2" if env["target"] == "Release" else "-O0"
            self.version = ""
            super().__init__(targets, env)
        def solveTarget(self, targets, env:dict):
            self.flags += ["-Wall", "-pedantic", "-march=native", "-pthread", "-Wno-unknown-pragmas", "-Wno-ignored-attributes", "-Wno-unused-local-typedefs"]
            self.flags += ["-m64" if env["platform"] == "x64" else "-m32"]
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
                self.flags = list(set(a + self.flags) - set(d))
                a,d = solveElementList(cxx, "defines", env)
                self.defines = list(set(a + self.defines) - set(d))
                a,d = solveElementList(cxx, "incpath", env)
                self.incpath = list(set(a + self.incpath) - set(d))
            super().solveTarget(targets, env)
            target = targets[self.prefix()]
            self.pch = target.get("pch", "")
            a,_ = solveElementList(target, "debug", env)
            self.debugLevel = a[0] if len(a)>0 else self.debugLevel
            a,_ = solveElementList(target, "optimize", env)
            self.optimize = a[0] if len(a)>0 else self.optimize
            self.version = target.get("version", self.langVersion())
            a,d = solveElementList(target, "defines", env)
            self.defines = list(set(a + self.defines) - set(d))
            a,d = solveElementList(target, "incpath", env)
            self.incpath = list(set(a + self.incpath) - set(d))
        def write(self, file):
            super().write(file)
            Project._writeItems(file, self.prefix()+"_defs", self.defines)
            Project._writeItems(file, self.prefix()+"_incpaths", self.incpath)
            Project._writeItems(file, self.prefix()+"_flags", [self.version, self.debugLevel, self.optimize], state="+")
            Project._writeItem(file, self.prefix()+"_pch", self.pch)

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
            Project._writeItems(file, self.prefix()+"_incpaths", self.incpath)
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
            Project._writeItems(file, self.prefix()+"_targets", self.targets)
            Project._writeItem(file, self.prefix()+"_flags", "--target="+(",".join(self.targets)), state="+")

    def __init__(self, data:dict, path:str):
        self.raw = data
        self.name = data["name"]
        self.type = data["type"]
        self.path = path[2:] if path.startswith("./") or path.startswith(".\\") else path
        self.dependency = []
        self.version = data.get("version", "")
        self.desc = data.get("description", "")
        libs = data.get("library", {})
        self.libStatic = libs.get("static", [])
        self.libDynamic = libs.get("dynamic", [])
        self.targets = []
        self.linkflags = []
        pass

    BTargets = list(getSubclasses(BuildTarget))
    def solveTarget(self, env:dict):
        if env["compiler"] == "clang" and env["target"] == "Release":
            self.linkflags += ["-fuse-ld=gold"]
        os.chdir(os.path.join(env["rootDir"], self.path))
        targets = self.raw.get("targets", [])
        self.targets = [t(targets, env) for t in Project.BTargets if t.prefix() in targets]
        os.chdir(env["rootDir"])

    def solveDependency(self, projs:dict):
        self.dependency.clear()
        for dep in self.raw.get("dependency", []):
            proj = projs.get(dep)
            if proj == None:
                raise Exception("missing dependency for {}".format(dep))
            self.dependency.append(proj)
        pass

    def writeMakefile(self, env:dict):
        deps = set(self.dependency)
        if self.type == "executable":
            checked = deps.copy()
            wanted = [proj for proj in self.dependency if proj.type == "dynamic"]
            nestedDeps = set()
            while len(wanted) > 0:
                p = wanted[0]
                dyndep = set(proj for proj in p.dependency if proj.type == "dynamic")
                nestedDeps |= dyndep
                newdep = dyndep - checked
                wanted.extend(newdep)
                checked |= newdep
                del wanted[0]
            deps = list(nestedDeps | deps)
        self.libStatic  += [proj.name for proj in deps if proj.type == "static"]
        self.libDynamic += [proj.name for proj in deps if proj.type == "dynamic"]

        objdir = os.path.join(env["rootDir"], self.path, env["objpath"])
        os.makedirs(objdir, exist_ok=True)
        with open(os.path.join(objdir, "xzbuild.proj.mk"), 'w') as file:
            file.write("# xzbuild per project file\n")
            file.write("# written at [{}]\n".format(time.asctime(time.localtime(time.time()))))
            file.write("\n\n# Project [{}]\n\n".format(self.name))
            self._writeItem(file, "NAME", self.name)
            self._writeItem(file, "BUILD_TYPE", self.type)
            self._writeItems(file, "LINKFLAGS", self.linkflags)
            self._writeItems(file, "libDynamic", self.libDynamic)
            self._writeItems(file, "libStatic", self.libStatic)
            for t in self.targets:
                t.write(file)

    def __repr__(self):
        d = {k:v for k,v in vars(self).items() if not (k == "raw" or k == "dependency")}
        return str(d)


def gatherProj():
    def readMetaFile(d:str):
        with open(os.path.join(d, "xzbuild.proj.json"), 'r') as f:
            return json.load(f)
    target = [(r, readMetaFile(r)) for r,d,f in os.walk(".") if "xzbuild.proj.json" in f]
    projects = {p.name:p for p in [Project(proj, d) for d,proj in target]}
    for proj in projects.values():
        proj.solveDependency(projects)
    return projects

def help():
    print("{0.white}build.py {0.cyan}<build|clean|buildall|cleanall|rebuild|rebuildall> <project> {0.magenta}[<Debug|Release>] [<x64|x86>]{0.clear}".format(COLOR))
    print("{0.white}build.py {0.cyan}<list|help>{0.clear}".format(COLOR))
    pass

def makeit(proj:Project, env:dict, action:str):
    # action = [build|clean|rebuild]
    rootDir = env["rootDir"]
    buildtype = "static library" if proj.type == "static" else ("dynamic library" if proj.type == "dynamic" else "executable binary")
    print("{clr.green}{} {clr.magenta}{}{clr.clear} [{clr.cyan}{}{clr.clear}] [{clr.magenta}{}{clr.clear} version on {clr.magenta}{}{clr.clear}] to {clr.green}{}{clr.clear}"\
        .format(action, buildtype, proj.name, env["target"], env["platform"], os.path.join(rootDir, env["objpath"]), clr=COLOR))
    proj.solveTarget(env)
    proj.writeMakefile(env)
    projDir = os.path.join(rootDir, proj.path)
    os.chdir(projDir)
    doClean = 0
    buildObj = ""
    if action == "build" or action == "rebuild":
        for t in proj.targets:
            t.printSources()
    if action == "clean" or action == "rebuild":
        doClean = 1
    if action == "clean":
        buildObj = "clean"
    cmd = "make {0} OBJPATH=\"{1}\" SOLPATH=\"{2}\" CLEAN={3} -f {2}/XZBuildMakeCore.mk -j4"
    cmd = cmd.format(buildObj, env["objpath"], rootDir, doClean)
    #print(cmd)
    ret = subprocess.call(cmd, shell=True) == 0
    os.chdir(rootDir)
    return ret

def listproj(projs:dict, projname: str):
    if projname == None:
        for proj in projs.values():
            print("{clr.green}[{}] {clr.magenta}({}) {clr.clear}{}\n{}".format(proj.name, proj.type, proj.version, proj.desc, clr=COLOR))
    else:
        def printDep(proj: Project, ends: tuple):
            prev = "".join(["   " if l else "│  " for l in ends[:-1]])
            cur = "" if len(ends)==1 else ("└──" if ends[-1] else "├──")
            print("{}{}{clr.green}[{}]{clr.magenta}({}){clr.clear}{}".format(prev, cur, proj.name, proj.type, proj.desc, clr=COLOR))
            newends = ends + (False,)
            for dep in proj.dependency[:-1]:
                printDep(dep, newends)
            for dep in proj.dependency[-1:]:
                printDep(dep, ends + (True,))
        printDep(projs[projname], (True,))
    pass

def mainmake(action:str, projs:set, env:dict):
    def genDependency(projs:set):
        solved = set()
        waiting = deque(projs)
        while len(waiting) > 0:
            target = waiting.popleft()
            for p in target.dependency:
                if p not in solved:
                    waiting.append(p)
            solved.add(target)
        builded = set()
        while len(solved) > 0:
            hasObj = False
            for p in solved:
                if set(p.dependency).issubset(builded):
                    yield p
                    builded.add(p)
                    solved.remove(p)
                    hasObj = True
                    break
            if not hasObj:
                raise Exception("some dependency can not be fullfilled")
        pass
    def sortDependency(projs:set):
        wanted = set(projs)
        while len(wanted) > 0:
            hasObj = False
            for p in wanted:
                if len(set(p.dependency).intersection(wanted)) == 0:
                    yield p
                    wanted.remove(p)
                    hasObj = True
                    break
            if not hasObj:
                raise Exception("some dependency can not be fullfilled")
        pass
    if action.endswith("all"):
        projs = [x for x in genDependency(projs)]
        action = action[:-3]
    else:
        projs = [x for x in sortDependency(projs)]
    print("build dependency:\t" + "->".join(["{clr.green}[{}]{clr.clear}".format(p.name, clr=COLOR) for p in projs]))
    writeEnv(env)
    suc = 0
    all = 0
    for proj in projs:
        b = makeit(proj, env, action)
        all += 1
        suc += 1 if b else 0
    return (suc, all)

def parseProj(proj:str, projs:dict):
    wanted = set()
    names = set(re.findall(r"[-.\w']+", proj)) # not keep removed items
    if "all" in names:
        names.update(projs.keys())
    if "all-dynamic" in names:
        names.update([pn for pn,p in projs.items() if p.type == "dynamic"])
    if "all-static" in names:
        names.update([pn for pn,p in projs.items() if p.type == "static"])
    if "all-executable" in names:
        names.update([pn for pn,p in projs.items() if p.type == "executable"])
    names.difference_update(["all", "all-dynamic", "all-static", "all-executable"]) # exclude special reserved items
    wantRemove = set([y for x in proj if x.startswith("-") for y in (x,x[1:])]) # exclude removed items
    names.difference_update(wantRemove)
    for x in names:
        if x in projs: wanted.add(projs[x])
        else: print("{clr.red}Unknwon project{clr.cyan}[{}]{clr.clear}".format(x, clr=COLOR))
    return wanted

def main(argv=None):
    try:
        action = argv[1]
        if action == "help":
            help()
            return 0

        projects = gatherProj()
        env = collectEnv()
        if len(argv) > 4: env["platform"] = argv[4]             
        if len(argv) > 3: env["target"] = argv[3]             
        env["objpath"] = ("{1}" if env["platform"] == "x86" else "{0}/{1}").format(env["platform"], env["target"])

        objproj = argv[2] if len(argv) > 2 else None

        if action == "test":
            for proj in projects.values():
                proj.solveTarget(env)
                print("{}\n{}\n\n".format(proj.name, str(proj)))
                proj.writeMakefile(env)
        elif action == "list":
            listproj(projects, objproj)
            return 0
        elif action in set(["build", "buildall", "clean", "cleanall", "rebuild", "rebuildall"]):
            projs = parseProj(objproj, projects)
            suc, all = mainmake(action, projs, env)
            preclr = COLOR.red if suc == 0 else COLOR.yellow if suc < all else COLOR.green
            print("{}build [{}/{}] successed.{clr.clear}".format(preclr, suc, all, clr=COLOR))
            return 0 if suc == all else -2
        else:
            raise IndexError()
    except IndexError:
        print("{clr.red}unknown action: {}{clr.clear}".format(argv[1:], clr=COLOR))
        help()
        return -1
    except KeyError:
        print("{clr.red}cannot find target project [{}]{clr.clear}".format(objproj, clr=COLOR))
        return -1
    pass

if __name__ == "__main__":
    osname = platform.system()
    if osname == "Windows":
        # print("{0.yellow}For Windows, use Visual Studio 2017 to build!{0.clear}".format(COLOR))
        # sys.exit(0)
        pass
    elif osname == "Darwin":
        print("{0.yellow}maxOS support is not tested!{0.clear}".format(COLOR))
    elif osname != "Linux":
        print("{0.yellow}unknown OS!{0.clear}".format(COLOR))
        sys.exit(-1)
    sys.exit(main(sys.argv))
