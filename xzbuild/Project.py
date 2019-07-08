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
            self.linkflags += ["-fuse-ld=gold"]
        os.chdir(os.path.join(env["rootDir"], self.path))
        targets = self.raw.get("targets", [])
        existTargets = [t for t in _AllTargets if t.prefix() in targets]
        self.targets = [t(targets, env) for t in existTargets]
        for t in existTargets:
            t.modifyProject(self, env)
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



