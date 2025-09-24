import json
import os
import platform
import time
import re
import subprocess
from collections import deque,OrderedDict
from enum import Flag
from . import writeItems,writeItem
from . import PowerList as PList
from .Target import _AllTargets


class ProjectType(Flag):
    Empty = 0
    Dynamic = 1
    Static = 2
    Executable = 4
    All = Dynamic | Static | Executable

def parseKVDataFromRegex(fpath:str, targets:list, rstr:str, keyIdx:int, valIdx:int) -> dict:
    with open(fpath, 'r') as file:
        pending = set(targets)
        result = {}
        for line in file:
            mth = re.match(rstr, line)
            if mth == None: continue
            key = mth.group(keyIdx)
            if key in pending:
                result[key] = mth.group(valIdx).strip('\"')
                pending.remove(key)
                if len(pending) == 0:
                    break
        return result

def parseKVData(dst:dict, req:dict, srcPath:str, buildPath:str):
    type = req["type"]
    fpath = os.path.normpath(os.path.join(srcPath if srcPath else buildPath, req["file"]))
    regexStr = ""
    keyIdx = 1
    valIdx = 2
    if type == "cmake":
        regexStr = r'^\s*set\s*\((\w+)\s+(.*)\)'
    elif type == "cxx":
        regexStr = r'^\s*#\s*define\s+(\w+)\s+([^\s\\]+)'
    elif type == "regex":
        regexStr = req["regex"]
        keyIdx = req.get("key", 1)
        valIdx = req.get("val", 2)
    if regexStr:
        targets = req["targets"]
        kvs = parseKVDataFromRegex(fpath, targets, regexStr, keyIdx, valIdx)
        dst.update(kvs)

class Project:
    def __init__(self, data:dict, path:str):
        #print(f"Project at [{path}]")
        self.raw = data
        self.name = data["name"]
        self.type = data["type"]
        if self.type == "executable": 
            self.typeFlag = ProjectType.Executable
        elif self.type == "static": 
            self.typeFlag = ProjectType.Static
        elif self.type == "dynamic":
            self.typeFlag = ProjectType.Dynamic
        else: 
            raise Exception(f"unrecognized project type [{self.type}]")
        self.buildPath = os.path.normpath(path)
        self.srcPath = os.path.normpath(os.path.join(path, data.get("srcPath", "")))
        self.dependency = []
        self.kvDict = {}
        for req in data.get("kvSource", []):
            parseKVData(self.kvDict, req, self.srcPath, self.buildPath)
        self.version = data.get("version", "")
        if isinstance(self.version, dict):
            if "format" in self.version:
                self.version = self.version["format"].format(**self.kvDict)
            elif "targets" in self.version:
                self.version = ".".join([self.kvDict.get(key, "0") for key in self.version["targets"]])
            else:
                self.version = ""
        self.desc = data.get("description", "")
        self.libs = data.get("library", {})
        self.libFlags = data.get("libflags", {})
        self.libStatic = []
        self.libDynamic = []
        self.libDirs = []
        self.tbds = data.get("framework", {})
        self.tbdLibs = []
        self.tbdDirs = []
        self.targets = []
        self.linkflags = []
        self.libVersion = data.get("libVersion", "")
        self.expmap = data.get("expmap", "")
        self.targetName = ""
        pass

    @staticmethod
    def procLib(target:list):
        newlist = []
        def proc(item:str):
            mth = re.findall(r"(\$pkg-config\(([a-zA-Z0-9-.]+)\))", item)
            if len(mth) > 0:
                key = mth[0][1]
                try:
                    ret = subprocess.run(["pkg-config", "--libs-only-l", key], capture_output=True, text=True)
                    print(f"pkg-config for [{key}] get: [{ret.stdout.strip()}]")
                    for x in ret.stdout.split():
                        if x.startswith("-l"):
                            newlist.append(x[2:])
                finally: pass
            else:
                newlist.append(item)
        for item in target:
            proc(item)
        return newlist

    def solveTargets(self, env:dict):
        def procLibEle(ret:tuple, ele:dict, env:dict):
            return tuple(list(env[i[1:]] if i.startswith("@") else i for i in x) for x in ret)
        
        osname = platform.system()
        self.linkflags  = PList.solveElementList(self.raw,  "linkflags", env)[0]
        self.libStatic  = Project.procLib(PList.solveElementList(self.libs, "static",  env, procLibEle)[0])
        self.libDynamic = Project.procLib(PList.solveElementList(self.libs, "dynamic", env, procLibEle)[0])
        self.libDirs    = PList.solveElementList(self.libs, "path", env)[0]
        self.linkflags += [f"-L{x}" for x in self.libDirs]
        if osname == 'Darwin':
            self.tbdLibs = PList.solveElementList(self.tbds, "framework",  env, procLibEle)[0]
            self.tbdDirs = PList.solveElementList(self.tbds, "path", env)[0]
            self.linkflags += [f"-F{x}" for x in self.tbdDirs]

        if self.type == "executable": 
            self.targetName = self.name
            if osname == 'Darwin':
                self.linkflags += ["-Wl,-rpath,.", "-Wl,-rpath,@executable_path"]
            else:
                self.linkflags += ["-Wl,-rpath,.", "-Wl,-rpath,'$$$$ORIGIN'"]
        elif self.type == "static": 
            self.targetName = f"lib{self.name}.a"
        elif self.type == "dynamic":
            if osname == 'Darwin':
                self.targetName = f"lib{self.name}.dylib"
                dynName = f"lib{self.name}.{self.libVersion}.dylib" if self.libVersion else self.targetName
                self.linkflags += ["-dynamiclib", "-Wl,-w", f"-Wl,-install_name,@rpath/{dynName}", "-Wl,-rpath,.", "-Wl,-rpath,@loader_path"]
            else:
                self.targetName = f"lib{self.name}.so"
                dynName = f"{self.targetName}.{self.libVersion}" if self.libVersion else self.targetName
                self.linkflags += ["-shared", f"-Wl,-soname,{dynName}", "-Wl,-rpath,.", "-Wl,-rpath,'$$$$ORIGIN'"]
        
        if self.expmap and not osname == 'Darwin':
            self.linkflags += [f"-Wl,--version-script,{self.expmap}"]

        if env["compiler"] == "clang" and not osname == 'Darwin':
            self.linkflags += ["-fuse-ld=lld"]

        os.chdir(os.path.join(env["rootDir"], self.srcPath))
        targets = self.raw.get("targets", {})
        existTargets = [t for t in _AllTargets if t.prefix() in targets]
        for t in existTargets:
            t.modifyProject(self, env)
        self.targets = [t(targets, self, env) for t in existTargets]
        os.chdir(env["rootDir"])

    def solveDependency(self, projs:"ProjectSet", env:dict):
        self.dependency.clear()
        deps = PList.solveElementList(self.raw, "dependency", env)[0]
        for dep in deps:
            proj = projs.get(dep)
            if proj == None:
                raise Exception(f"missing dependency [{dep}] for [{self.name}]")
            self.dependency.append(proj)
        pass

    def writeMakefile(self, env:dict):

        deps = set(self.dependency)
        depStatic  = ProjectSet.sortByDependency([proj for proj in deps if proj.type == "static"], ProjectType.Static)
        depStatic.reverse()
        depDynamic = ProjectSet.sortByDependency([proj for proj in deps if proj.type == "dynamic"])
        depDynamic.reverse()
        libStatic  = [proj.name for proj in depStatic ] + self.libStatic
        libDynamic = [proj.name for proj in depDynamic] + self.libDynamic

        osname = platform.system()
        allPfx = "-Wl,-all_load" if osname == 'Darwin' else "-Wl,--whole-archive"
        allSfx = "-Wl,-noall_load" if osname == 'Darwin' else "-Wl,--no-whole-archive"
        def procStatic(name, flags):
            flag = flags.get(name, {})
            return f"{allPfx} -l{name} {allSfx}" if flag.get("all", False) else f"-l{name}"
        
        linkLibs = []
        if len(libStatic) > 0:
            linkLibs += [procStatic(x, self.libFlags) for x in libStatic]
        if env["stdlib"] == "libc++":
            if "android" in env:
                if env["ndkVer"] >= 2300:
                    """
                    On NDK >= 23, all architecture now uses libunwind
                    See https://github.com/android/ndk/issues/1230
                    See https://github.com/android/ndk/wiki/Changelog-r23#changes
                    """
                    linkLibs.append("-lunwind")
                elif env["arch"] == "arm" and env["bits"] == 32:
                    """
                    On Android armv7, use of _Unwind_Backtrace may cause segment fault because of in compatible layout of
                    base-class AbstractUnwindCursor and actual type _Unwind_Context.
                    See https://lists.llvm.org/pipermail/cfe-dev/2018-February/057014.html
                    Use of stacktrace crashes at `unw_set_reg`, see https://android.googlesource.com/platform/external/libunwind_llvm/+/refs/heads/ndk-release-r21/src/libunwind.cpp
                    See blog https://zhuanlan.zhihu.com/p/33937283
                    See google's suggestion on NDK: https://android.googlesource.com/platform/ndk/+/master/docs/BuildSystemMaintainers.md#Unwinding
                    """
                    linkLibs.append("-lunwind")
            else:
                linkLibs.remove("-lunwind")
        linkLibs += [f"-l{x}" for x in libDynamic]
        if osname == 'Darwin':
            linkLibs += [f"-framework {x}" for x in self.tbdLibs]
        
        objdir = os.path.join(env["rootDir"], self.buildPath, env["objpath"])
        os.makedirs(objdir, exist_ok=True)
        with open(os.path.join(objdir, "xzbuild.proj.mk"), 'w') as file:
            file.write("# xzbuild per project file\n")
            file.write("# written at [{}]\n".format(time.asctime(time.localtime(time.time()))))
            file.write("\n\n# Project [{}]\n\n".format(self.name))
            writeItem (file, "NAME",        self.name)
            writeItem (file, "TARGET_NAME", self.targetName)
            writeItem (file, "BUILD_TYPE",  self.type)
            writeItems(file, "LINKFLAGS",   self.linkflags)
            writeItems(file, "LINKLIBS",    linkLibs)
            writeItems(file, "libDynamic",  libDynamic)
            writeItems(file, "libStatic",   libStatic)
            writeItems(file, "libVersion",  self.libVersion)
            # writeItems(file, "xz_libDir",   self.libDirs, state="+")
            baseDirs = set()
            for t in self.targets:
                baseDirs.update(t.baseDirs)
            if len(baseDirs) > 0:
                writeItems(file, "VPATH", baseDirs)
            for t in self.targets:
                t.write(file)

    def __repr__(self):
        d = {k:v for k,v in vars(self).items() if not (k == "raw" or k == "dependency")}
        return str(d)


class ProjectSet:
    def __init__(self, data=None):
        self._data = {}
        self.__add__(data)
    def __getitem__(self, key):
        return self._data[key]
    def __delitem__(self, item):
        name = item.name if item is Project else item
        del self._data[name]
    def __contains__(self, item):
        return item in self._data.values() if item is Project else item in self._data
    def __add__(self, data):
        if data is None:
            pass
        if type(data) is Project:
            if data.name in self._data: raise RuntimeError(f"project {data.name} being added already exists")
            else: self._data[data.name] = data
        else:
            for item in data: self.__add__(item)
    def __iter__(self):
        return iter(self._data.values())
    def __len__(self):
        return len(self._data)
    def __repr__(self):
        return f"{type(self).__name__}({self._data})"
    def get(self, key, val=None) -> Project:
        return self._data.get(key, val)
    
    def names(self):
        return self._data.keys()

    def solveDependency(self, env:dict):
        for proj in self._data.values():
            proj.solveDependency(self, env)

    def sortDependency(self, projs, addDepend:ProjectType = ProjectType.Empty) -> list:
        return ProjectSet.sortByDependency(projs, addDepend, self)
    
    @staticmethod
    def sortByDependency(projs, addDepend:ProjectType = ProjectType.Empty, self:"ProjectSet" = None) -> list:
        wanted = OrderedDict()
        for x in projs:
            proj = x if type(x) is Project else None if self is None else self._data.get(x, None)
            if proj is None: raise Exception(f"{x} not recoginized as a project")
            wanted[proj] = None
        if addDepend != ProjectType.Empty: 
            waiting = deque(wanted.keys())
            while len(waiting) > 0:
                target = waiting.popleft()
                for p in target.dependency:
                    if p not in wanted and bool(addDepend & p.typeFlag):
                        waiting.append(p)
                wanted[target] = None
        satified = []
        while len(wanted) > 0:
            hasObj = False
            for p in wanted.keys():
                if not any([x in wanted for x in p.dependency]):
                    satified.append(p)
                    wanted.pop(p)
                    hasObj = True
                    break
            if not hasObj:
                raise Exception(f"some dependency can not be fullfilled: {wanted.keys()}")
        return satified

    @staticmethod
    def gatherFrom(dir:str="."):
        def readMetaFile(d:str):
            with open(os.path.join(d, "xzbuild.proj.json"), 'r') as f:
                return json.load(f)
        target = [(r, readMetaFile(r)) for r,d,f in os.walk(dir) if "xzbuild.proj.json" in f]
        projects = ProjectSet([Project(proj, d) for d,proj in target])
        return projects

    @staticmethod
    def loadSolution(env: dict):
        solDir = env["rootDir"]
        def combinePath(sol: dict, field: str, env: dict):
            a,d = PList.solveElementList(sol, field, env)
            a = [os.path.normpath(x if os.path.isabs(x) else os.path.join(solDir, x)) for x in a]
            d = [os.path.normpath(x if os.path.isabs(x) else os.path.join(solDir, x)) for x in d]
            env[field] = PList.combineElements(env[field], a, d)
        def procEnvEle(ret:tuple, ele:dict, env:dict):
            if "key" in ele:
                key = ele["key"]
                return tuple(list((key, i) for i in x) for x in ret)
            return ret
        def addEnv(env:dict, target, field):
            a,d = PList.solveElementList(target, field, env, procEnvEle)
            adds = PList.combineElements([], a, d)
            for ele in adds:
                print(f"add:{ele}")
                if isinstance(ele, tuple):
                    env[ele[0]] = ele[1]
                else:
                    env[ele] = "1"

        solFile = os.path.join(solDir, "xzbuild.sol.json")
        if os.path.isfile(solFile):
            with open(solFile, 'r') as f:
                solData = json.load(f)
            for x in solData.get("environments", []):
                addEnv(env, x, None)
            combinePath(solData, "incDirs", env)
            combinePath(solData, "libDirs", env)
            a,d = PList.solveElementList(solData, "defines", env)
            env["defines"] = PList.combineElements([], a, d)

        usrFile = os.path.join(solDir, "xzbuild.user.json")
        if os.path.isfile(usrFile):
            with open(usrFile, 'r') as f:
                usrData = json.load(f)
            for x in usrData.get("environments", []):
                addEnv(env, x, None)
            combinePath(usrData, "incDirs", env)
            combinePath(usrData, "libDirs", env)
            a,d = PList.solveElementList(usrData, "defines", env)
            env["defines"] = PList.combineElements([], a, d)
                

