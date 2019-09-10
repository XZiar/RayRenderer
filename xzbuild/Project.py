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
from collections.abc import MutableMapping
from ._Rely import writeItems,writeItem
from .Target import _AllTargets


class Project:
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
        self.libDirs = []
        pass

    def solveTarget(self, env:dict):
        if env["compiler"] == "clang" and env["target"] == "Release":
            self.linkflags += ["-fuse-ld=lld"]
        os.chdir(os.path.join(env["rootDir"], self.path))
        targets = self.raw.get("targets", [])
        existTargets = [t for t in _AllTargets if t.prefix() in targets]
        self.targets = [t(targets, env) for t in existTargets]
        for t in existTargets:
            t.modifyProject(self, env)
        os.chdir(env["rootDir"])

    def solveDependency(self, projs:"ProjectSet"):
        self.dependency.clear()
        for dep in self.raw.get("dependency", []):
            proj = projs.get(dep)
            if proj == None:
                raise Exception(f"missing dependency {dep} for {self.name}")
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
            writeItem(file, "NAME", self.name)
            writeItem(file, "BUILD_TYPE", self.type)
            writeItems(file, "LINKFLAGS", self.linkflags)
            writeItems(file, "libDynamic", self.libDynamic)
            writeItems(file, "libStatic", self.libStatic)
            writeItems(file, "xz_libDir", self.libDirs, state="+")
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

    def solveDependency(self):
        for proj in self._data.values():
            proj.solveDependency(self)
    
    @staticmethod
    def gatherFrom(dir:str="."):
        def readMetaFile(d:str):
            with open(os.path.join(d, "xzbuild.proj.json"), 'r') as f:
                return json.load(f)
        target = [(r, readMetaFile(r)) for r,d,f in os.walk(dir) if "xzbuild.proj.json" in f]
        projects = ProjectSet([Project(proj, d) for d,proj in target])
        return projects
