import glob
import json
import os
import platform
import re
import sys
from collections import deque
from subprocess import call

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

class TrackFile:
    def __init__(self, arg):
        if isinstance(arg, TrackFile):
            self.path = arg.path
            self.mtime = arg.mtime
        else:
            self.path = arg
            self.mtime = os.path.getmtime(arg)

class CXXFile(TrackFile):
    def __init__(self, file:TrackFile):
        TrackFile.__init__(file)
        

class BaseTarget:
    def InitSource(self, dir:str, data:dict):
        srcfiles = set()
        for item in data.get("src", []):
            files = glob.glob(os.path.join(dir, item), recursive=True)
            srcfiles += files
        for item in data.get("except", []):
            files = glob.glob(os.path.join(dir, item), recursive=True)
            srcfiles -= files
        self.srcs = { file:TrackFile(file) for file in srcfiles }
    def FilterNewSource(self, cache:dict):
        return [f for fn,f in self.srcs if fn not in cache or cache[fn].mtime != f.mtime]

class CppTarget(BaseTarget):
    def __init__(self, dir:str, data:dict):
        BaseTarget.InitSource(self, dir, data)


class Project1:
    def __init__(self, dir:str):
        with open(os.path.join(dir, "xzbuild.proj.json"), 'r') as f:
            data = json.load(f)
        self.name = data["name"]
        self.type = data["type"]
        self.deps = data["dependency"]
        self.version = data.get("version", "")
        self.desc = data.get("description", "")
        self.dependency = None
        self.targets = {}
        if "cpp" in data["targets"]:
            self.targets["cpp"] = CppTarget(dir, data["cpp"])
        pass

    def solve(self, projs:dict):
        self.dependency = []
        for dep in self.deps:
            proj = projs.get(dep)
            if proj == None:
                raise Exception("missing dependency [{}] for [{}]".format(dep, self.name))
            self.dependency.append(proj)
        pass
