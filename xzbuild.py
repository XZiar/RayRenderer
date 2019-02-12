import glob
import json
import os
import platform
import re
import subprocess
import sys
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

projDir = os.getcwd()
depDir  = os.environ.get("CPP_DEPENDENCY_PATH", "./")

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
    env = {"target": "Debug", "platform": "x64"}
    compiler = os.environ.get("CPPCOMPILER", "g++")
    rawdefs = subprocess.check_output("{} -dM -E - < /dev/null".format(compiler), shell=True)
    defs = set([d.split()[1] for d in rawdefs.decode().splitlines()])
    env["intrin"] = set(i[1] for i in intrinMap.items() if i[0] in defs)
    env["compiler"] = "clang" if "__clang__" in defs else "gcc"
    return env

def solveElement(element, env:dict) -> tuple:
    if element is list:
        return (element, [])
    if element is not dict:
        return ([element], [])
    def checkMatch(obj:dict, name:str, test) -> bool:
        target = obj.get(name)
        if target is None:
            return True
        if target is list:
            return all([test(t) for t in target])
        if target is dict:
            return all([test(t) for t in target.items()])
        else:
            return test(target)
    tests = \
    {
        "ifhas": lambda x: x in env,
        "ifno" : lambda x: x not in env,
        "ifeq" : lambda x: env.get(x[0]) == x[1],
        "ifneq": lambda x: env.get(x[0]) != x[1],
        "ifin" : lambda x: x[1] in env.get(x[0], []),
        "ifnin": lambda x: x[1] not in env.get(x[0], []),
    }
    if all(checkMatch(element, n, t) for n,t in tests):
        return (element.get("+", []), element.get("-", []))
    return ([], [])

class Project:
    def __init__(self, data:dict, path:str):
        self.name = data["name"]
        self.type = data["type"]
        self.path = path[2:] if path.startswith("./") or path.startswith(".\\") else path
        self.deps = data.get("dependency", [])
        self.dependency = []
        self.version = data.get("version", "")
        self.desc = data.get("description", "")
        libs = data.get("library", {})
        self.libStatic = libs.get("static", [])
        self.libDynamic = libs.get("dynamic", [])
        self.srcs = data.get("sources", {})
        self.sources = {"cpp": ["*.cpp"], "asm": ["*.S"], "nasm": ["*.asm"], "ispc": ["*.ispc"], "rc": ["*.rc"]}
        pass

    def solveDependency(self, projs:dict):
        self.dependency.clear()
        for dep in self.deps:
            proj = projs.get(dep)
            if proj == None:
                raise Exception("missing dependency for {}".format(dep))
            self.dependency.append(proj)
        self.libStatic  += [proj.name for proj in self.dependency if proj.type == "static"]
        self.libDynamic += [proj.name for proj in self.dependency if proj.type == "dynamic"]
        pass

    def solveSources(self, env:dict):
        curdir = os.getcwd()
        os.chdir(self.path)
        for target in ["cpp", "asm", "nasm", "ispc", "rc"]:
            if target not in self.srcs:
                self.sources[target] = []
            else:
                middle = list(solveElement(ele, env) for ele in self.srcs[target])
                adds = set(f for a,_ in middle for i in a for f in glob.glob(i))
                dels = set(f for _,d in middle for i in d for f in glob.glob(i))
                self.sources[target] = list(adds - dels)
        os.chdir(curdir)

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

def build(rootDir:str, proj:Project, args:dict):
    targetPath = os.path.join(rootDir, proj.path)
    print("build {clr.green}[{}]{clr.clear} at [{}]".format(proj.name, targetPath, clr=COLOR))
    os.chdir(targetPath)
    projPath = os.path.relpath(rootDir, targetPath) + "/"
    cmd = "make BOOST_PATH=\"{}/include\" PLATFORM={} TARGET={} PROJPATH=\"{}\" -j4".format(depDir, args["platform"], args["target"], projPath)
    retcode = subprocess.call(cmd, shell=True)
    os.chdir(rootDir)
    return retcode == 0

def clean(rootDir:str, proj:Project, args:dict):
    targetPath = os.path.join(rootDir, proj.path)
    print("clean {clr.green}[{}]{clr.clear} at [{}]".format(proj.name, targetPath, clr=COLOR))
    os.chdir(targetPath)
    projPath = os.path.relpath(rootDir, targetPath) + "/"
    cmd = "make clean PLATFORM={} TARGET={} PROJPATH=\"{}\"".format(args["platform"], args["target"], projPath)
    retcode = subprocess.call(cmd, shell=True)
    os.chdir(rootDir)
    return retcode == 0

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
    return False

def mainmake(action:str, projs:set, args:dict):
    if action.endswith("all"):
        projs = [x for x in genDependency(projs)]
        action = action[:-3]
    else:
        projs = [x for x in sortDependency(projs)]
    print("build dependency:\t" + "->".join(["{clr.green}[{}]{clr.clear}".format(p.name, clr=COLOR) for p in projs]))
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
    names = set(re.findall(r"[-.\w']+", proj))
    if "all" in names:
        names.update(projs.keys())
    if "all-dynamic" in names:
        names.update([pn for pn,p in projs.items() if p.type == "dynamic"])
    if "all-static" in names:
        names.update([pn for pn,p in projs.items() if p.type == "static"])
    if "all-executable" in names:
        names.update([pn for pn,p in projs.items() if p.type == "executable"])
    names.difference_update(["all", "all-dynamic", "all-static", "all-executable"])
    wantRemove = set([y for x in names if x.startswith("-") for y in (x,x[1:]) ])
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

        objproj = argv[2] if len(argv) > 2 else None

        if action == "list":
            listproj(projects, objproj)
            return 0
        elif action in set(["build", "buildall", "clean", "cleanall", "rebuild", "rebuildall"]):
            proj = parseProj(objproj, projects)
            args = { "platform": "x64", "target": "Debug" }
            if len(argv) > 4: args["platform"] = argv[4]             
            if len(argv) > 3: args["target"] = argv[3]             
            suc, all = mainmake(action, proj, args)
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
    projects = gatherProj()
    env = collectEnv()
    for proj in projects.values():
        proj.solveSources(env)
        print("{}\n{}\n\n", proj.name, vars(proj))
    if osname == "Windows":
        # listproj(projects, None)
        print("{0.yellow}For Windows, use Visual Studio 2017 to build!{0.clear}".format(COLOR))
        sys.exit(0)
    elif osname == "Darwin":
        print("{0.yellow}maxOS support is not tested!{0.clear}".format(COLOR))
    elif osname != "Linux":
        print("{0.yellow}unknown OS!{0.clear}".format(COLOR))
        sys.exit(-1)
    sys.exit(main(sys.argv))
