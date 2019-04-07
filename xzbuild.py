#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import glob
import json
import os
import platform
import re
import subprocess
import sys
import time
from collections import deque

from xzbuild._Rely import COLOR
from xzbuild.Project import Project
from xzbuild.Environment import *

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
