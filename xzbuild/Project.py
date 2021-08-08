import json
import os
import time
from . import writeItems,writeItem
from . import PowerList as PList
from .Target import _AllTargets


class Project:
    def __init__(self, data:dict, path:str):
        #print(f"Project at [{path}]")
        self.raw = data
        self.name = data["name"]
        self.type = data["type"]
        self.buildPath = os.path.normpath(path)
        self.srcPath = os.path.normpath(os.path.join(path, data.get("srcPath", "")))
        self.dependency = []
        self.version = data.get("version", "")
        self.desc = data.get("description", "")
        self.libs = data.get("library", {})
        self.libStatic = []
        self.libDynamic = []
        self.targets = []
        self.linkflags = data.get("linkflags", [])
        self.libDirs = []
        self.expmap = data.get("expmap", "")
        if self.expmap:
            self.linkflags += [f"-Wl,--version-script -Wl,{self.expmap}"]
        pass

    def solveTargets(self, env:dict):
        self.libStatic  = PList.solveElementList(self.libs, "static",  env)[0]
        self.libDynamic = PList.solveElementList(self.libs, "dynamic", env)[0]
        if env["compiler"] == "clang" and not env.get("iOS", False):
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
        def solveNestedDeps(deps:set, dtype:str):
            checked = deps.copy()
            wanted = [proj for proj in deps if proj.type == dtype]
            nestedDeps = set()
            while len(wanted) > 0:
                p = wanted[0]
                matchDeps = set(proj for proj in p.dependency if proj.type == dtype)
                nestedDeps |= matchDeps
                newDeps = matchDeps - checked
                wanted.extend(newDeps)
                checked |= newDeps
                del wanted[0]
            return nestedDeps

        deps = set(self.dependency)
        nestedStatic  = solveNestedDeps(deps, "static")  if self.type != "static"     else set()
        nestedDynamic = solveNestedDeps(deps, "dynamic") if self.type == "executable" else set()

        deps = list(deps | nestedStatic | nestedDynamic)
        self.libStatic  += [proj.name for proj in deps if proj.type == "static"]
        self.libDynamic += [proj.name for proj in deps if proj.type == "dynamic"]

        objdir = os.path.join(env["rootDir"], self.buildPath, env["objpath"])
        os.makedirs(objdir, exist_ok=True)
        with open(os.path.join(objdir, "xzbuild.proj.mk"), 'w') as file:
            file.write("# xzbuild per project file\n")
            file.write("# written at [{}]\n".format(time.asctime(time.localtime(time.time()))))
            file.write("\n\n# Project [{}]\n\n".format(self.name))
            writeItem (file, "NAME",        self.name)
            writeItem (file, "BUILD_TYPE",  self.type)
            writeItems(file, "LINKFLAGS",   self.linkflags)
            writeItems(file, "libDynamic",  self.libDynamic)
            writeItems(file, "libStatic",   self.libStatic)
            writeItems(file, "xz_libDir",   self.libDirs, state="+")
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
    def get(self, key, val=None):
        return self._data.get(key, val)
    
    def names(self):
        return self._data.keys()

    def solveDependency(self, env:dict):
        for proj in self._data.values():
            proj.solveDependency(self, env)
    
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
        solFile = os.path.join(solDir, "xzbuild.sol.json")
        if os.path.isfile(solFile):
            with open(solFile, 'r') as f:
                solData = json.load(f)
            a,_ = PList.solveElementList(solData, "environment", env, procEnvEle)
            for ele in a:
                print(f"add:{ele}")
                if isinstance(ele, tuple):
                    env[ele[0]] = ele[1]
                else:
                    env[ele] = "1"
            combinePath(solData, "incDirs", env)
            combinePath(solData, "libDirs", env)
            a,d = PList.solveElementList(solData, "defines", env)
            env["defines"] = PList.combineElements([], a, d)
                

