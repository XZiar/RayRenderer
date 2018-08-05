import json
import os
import platform
import re
import sys
from collections import deque
from subprocess import call

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
    retcode = call(cmd, shell=True)
    os.chdir(rootDir)
    return retcode == 0

def clean(rootDir:str, proj:Project, args:dict):
    targetPath = os.path.join(rootDir, proj.path)
    print("clean \033[92m[{}]\033[39m at [{}]".format(proj.name, targetPath))
    os.chdir(targetPath)
    projPath = os.path.relpath(rootDir, targetPath) + "/"
    cmd = "make clean PROJPATH=\"{}\"".format(projPath)
    retcode = call(cmd, shell=True)
    os.chdir(rootDir)
    return retcode == 0

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

def makeone(action:str, proj:Project, args:dict):
    if action == "build":
        return build(projDir, proj, args)
    elif action == "clean":
        return clean(projDir, proj, args)
    elif action == "rebuild":
        b1 = clean(projDir, proj, args)
        b2 = build(projDir, proj, args)
        return b1 and b2
    return false

def mainmake(action:str, projs:set, args:dict):
    if action.endswith("all"):
        projs = [x for x in genDependency(projs)]
        action = action[:-3]
    else:
        projs = [x for x in sortDependency(projs)]
    print("build dependency:\t" + "->".join(["\033[92m[" + p.name + "]\033[39m" for p in projs]))
    os.makedirs("./Debug", exist_ok=True)
    os.makedirs("./Release", exist_ok=True)
    os.makedirs("./x64/Debug", exist_ok=True)
    os.makedirs("./x64/Release", exist_ok=True)
    suc = 0
    all = 0
    for proj in projs:
        b = makeone(action, proj, args)
        all += 1
        suc += 1 if b else 0
    return (suc, all)

def parseProj(proj:str, projs:dict):
    wanted = set()
    names = set(re.findall(r"[-\w']+", proj))
    if "all" in names:
        names.update(projs.keys())
    names.discard("all")
    wantRemove = set([y for x in names if x.startswith("-") for y in (x,x[1:]) ])
    names.difference_update(wantRemove)
    for x in names:
        if x in projs: wanted.add(projs[x])
        else: print("\033[91mUnknwon project\033[96m[{}]\033[39m".format(x))
    return wanted

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
            return 0
        elif action in set(["build", "buildall", "clean", "cleanall", "rebuild", "rebuildall"]):
            proj = parseProj(objproj, projects)
            args = { "platform": "x64", "target": "Debug" }
            if len(argv) > 4: args["platform"] = argv[3]             
            if len(argv) > 3: args["target"] = argv[4]             
            suc, all = mainmake(action, proj, args)
            clr = "\033[91m" if suc == 0 else "\033[93m" if suc < all else "\033[92m"
            print("{}build [{}/{}] successed.\033[39m".format(clr, suc, all))
            return suc == all
        else:
            raise IndexError()
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
