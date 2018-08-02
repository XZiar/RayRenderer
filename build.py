import json
import os
import platform
import sys
from collections import deque
from subprocess import call
from typing import Union

projDir = os.getcwd()
depDir  = os.environ.get("CPP_DEPENDENCY_PATH", "./")

class Project:
    def __init__(self, data:dict):
        self.name = data["name"]
        self.type = data["type"]
        self.path = data["path"]
        self.deps = data["dependency"]
        self.version = data.get("version", "")
        self.desc = data.get("description", "")
        self.dependency = []
        pass

    def solve(self, projs:dict):
        self.dependency.clear()
        for dep in self.deps:
            proj = projs.get(dep)
            if proj == None:
                raise Exception("missing dependency for {}".format(dep))
            self.dependency.append(proj)
        pass

def help():
    print("\033[97mbuild.py \033[96m<build|clean|buildall|cleanall|rebuild|rebuildall> <project> \033[95m[<Debug|Release>] [<x64|x86>]\033[39m")
    print("\033[97mbuild.py \033[96m<list|help>\033[39m")
    pass

def build(rootDir:str, proj:Project, args:dict):
    targetPath = os.path.join(rootDir, proj.path)
    print("build \033[92m[{}]\033[39m at [{}]".format(proj.name, targetPath))
    os.chdir(targetPath)
    projPath = os.path.relpath(rootDir, targetPath) + "/"
    cmd = "make BOOST_PATH=\"{}/include\" PLATFORM={} TARGET={} PROJPATH=\"{}\" -j4".format(depDir, args["platform"], args["target"], projPath)
    call(cmd, shell=True)
    os.chdir(rootDir)
    pass

def clean(rootDir:str, proj:Project, args:dict):
    targetPath = os.path.join(rootDir, proj.path)
    print("clean \033[92m[{}]\033[39m at [{}]".format(proj.name, targetPath))
    os.chdir(targetPath)
    projPath = os.path.relpath(rootDir, targetPath) + "/"
    cmd = "make clean PROJPATH=\"{}\"".format(projPath)
    call(cmd, shell=True)
    os.chdir(rootDir)
    pass

def listproj(projs:dict, projname: str):
    if projname == None:
        for proj in projs.values():
            print("\033[92m[{}] \033[95m({}) \033[39m{}\n{}".format(proj.name, proj.type, proj.version, proj.desc))
    else:
        def printDep(proj: Project, level: int):
            prefix = "|  "*(level-1) + ("" if level==0 else "|->")
            print("{}\033[92m[{}]\033[95m({})\033[39m{}".format(prefix, proj.name, proj.type, proj.desc))
            for dep in proj.dependency:
                printDep(dep, level+1)
        printDep(projs[projname], 0)
    pass

def genDependency(proj:Union[Project,set]):
    solved = set()
    if isinstance(proj, Project):
        waiting = deque([proj])
        while len(waiting) > 0:
            target = waiting.popleft()
            for p in target.dependency:
                if p not in solved:
                    waiting.append(p)
            solved.add(target)
    elif isinstance(proj, set):
        solved = proj
    else:
        raise TypeError()
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

def mainmake(action:str, proj:Union[Project,set], args:dict):
    if action.endswith("all"):
        projs = [x for x in genDependency(proj)]
        print("build dependency:\t" + "->".join(["\033[92m[" + p.name + "]\033[39m" for p in projs]))
        newaction = action[:-3]
        for p in projs:
            mainmake(newaction, p, args)
    else:
        if action == "build":
            build(projDir, proj, args)
        elif action == "clean":
            clean(projDir, proj, args)
        elif action == "rebuild":
            clean(projDir, proj, args)
            build(projDir, proj, args)

def main(argv=None):
    try:
        action = argv[1]
        if action == "help":
            help()
            return 0
        
        try:
            with open('xzbuild.proj.json', 'r') as f:
                data = json.load(f)
            projects = {p.name:p for p in [Project(proj) for proj in data["projects"]]}
            for proj in projects.values():
                proj.solve(projects)
        except IOError as e:
            print("\033[91munavaliabe project data [xzbuild.proj.json]", e.strerror)
            return -1
        except Exception as e:
            print(e)
            return -1
        
        objproj = argv[2] if len(argv) > 2 else None

        if action == "list":
            listproj(projects, objproj)
        elif action in set(["build", "clean", "buildall", "cleaall", "rebuild", "rebuildall"]):
            if objproj == "all":
                action = action if action.endswith("all") else action+"all"
                proj = set(projects.values())
            else:
                proj = projects[objproj]
            args = { "platform": "x64", "target": "Debug" }
            if len(argv) > 4: args["platform"] = argv[3]             
            if len(argv) > 3: args["target"] = argv[4]             
            mainmake(action, proj, args)
        else:
            raise IndexError()
        return 0
    except IndexError:
        print("\033[91munknown action: {}\033[39m".format(argv[1:]))
        help()
        return -1
    except KeyError:
        print("\033[91mcannot find target project [{}]\033[39m".format(objproj))
        return -1
    pass

if __name__ == "__main__":
    osname = platform.system()
    if osname == "Windows":
        print("\033[93mFor Windows, use Visual Studio 2017 to build!\033[39m")
        sys.exit(0)
    elif osname == "Darwin":
        print("\033[93mmaxOS support is not tested!\033[39m")
    elif osname != "Linux":
        print("\033[91munknown OS!\033[39m")
        sys.exit(-1)
    sys.exit(main(sys.argv))
