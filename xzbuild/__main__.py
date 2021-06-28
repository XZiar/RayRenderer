#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import re
import subprocess
import sys
from collections import deque

# enable package import when executed directly
if not __package__:
    path = os.path.join(os.path.dirname(__file__), os.pardir)
    sys.path.insert(0, path)

from xzbuild import COLOR
from xzbuild.Target import _AllTargets
from xzbuild.Project import Project, ProjectSet
from xzbuild.Environment import collectEnv, writeEnv

def help():
    print(f"{COLOR.white}python3 xzbuild {COLOR.cyan}<build|clean|buildall|cleanall|rebuild|rebuildall> <project,[project]|all> "
          f"{COLOR.magenta}[<Debug|Release>] [<x64|x86|ARM64|ARM>]{COLOR.clear}")
    print(f"{COLOR.white}python3 xzbuild {COLOR.cyan}<list> {COLOR.magenta}[<project>]{COLOR.clear}")
    print(f"{COLOR.white}python3 xzbuild {COLOR.cyan}<help>{COLOR.clear}")
    pass

def makeit(proj:Project, env:dict, action:str):
    # action = [build|clean|rebuild]
    rootDir = env["rootDir"]
    xzbuildPath = env["xzbuildPath"]
    objPath = env["objpath"]
    buildtype = "static library" if proj.type == "static" else ("dynamic library" if proj.type == "dynamic" else "executable binary")
    print(f'{COLOR.Green(action)} {COLOR.Magenta(buildtype)} [{COLOR.Cyan(proj.name)}] '
          f'[{COLOR.Magenta(env["target"])} version on {COLOR.Magenta(env["platform"])}] '
          f'at SOL/{COLOR.Green(proj.buildPath)} '
          f'to SOL/{COLOR.Green(objPath)}')
    proj.solveTargets(env)
    proj.writeMakefile(env)
    srcDir = os.path.join(rootDir, proj.srcPath)
    os.chdir(srcDir)
    doClean = 0
    buildObj = ""
    if env["verbose"] and (action == "build" or action == "rebuild"):
        for t in proj.targets:
            t.printSources()
    if action == "clean" or action == "rebuild":
        doClean = 1
    if action == "clean":
        buildObj = "clean"
    cmd = f'make {buildObj} SOLDIR="{rootDir}" OBJPATH="{objPath}" BUILDPATH="{proj.buildPath}" CLEAN={doClean} -f {rootDir}/{xzbuildPath}/XZBuildMakeCore.mk -j{env["threads"]} VERBOSE={1 if env["verbose"] else 0}'
    #print(cmd)
    ret = subprocess.call(cmd, shell=True) == 0
    os.chdir(rootDir)
    return ret

def listproj(projs: ProjectSet, projname: str):
    if projname == None:
        for proj in projs:
            print(f"{COLOR.green}[{proj.name}] {COLOR.magenta}({proj.type}) {COLOR.clear}{proj.version}\n{proj.desc}")
    else:
        def printDep(proj: Project, ends: tuple):
            prev = "".join(["   " if l else "│  " for l in ends[:-1]])
            cur = "" if len(ends)==1 else ("└──" if ends[-1] else "├──")
            print(f"{prev}{cur}{COLOR.green}[{proj.name}]{COLOR.magenta}({proj.type}){COLOR.clear}{proj.desc}")
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
    print(f"run {env['threads']} threads on {env['cpuCount']} cores")
    print("build dependency:\t" + "->".join([f"{COLOR.green}[{p.name}]{COLOR.clear}" for p in projs]))
    writeEnv(env)
    suc = 0
    all = 0
    for proj in projs:
        b = makeit(proj, env, action)
        all += 1
        suc += 1 if b else 0
    return (suc, all)

def parseProj(proj:str, projs:ProjectSet):
    wanted = set()
    names = set(re.findall(r"[-.\w']+", proj)) # keep removed items
    if "all" in names:
        names.update(projs.names())
    if "all-dynamic" in names:
        names.update([p.name for p in projs if p.type == "dynamic"])
    if "all-static" in names:
        names.update([p.name for p in projs if p.type == "static"])
    if "all-executable" in names:
        names.update([p.name for p in projs if p.type == "executable"])
    names.difference_update(["all", "all-dynamic", "all-static", "all-executable"]) # exclude special reserved items
    wantRemove = set([y for x in names if x.startswith("-") for y in (x,x[1:])]) # exclude removed items
    names.difference_update(wantRemove)
    for x in names:
        if x in projs: wanted.add(projs[x])
        else: print(f"{COLOR.red}Unknwon project{COLOR.cyan}[{x}]{COLOR.clear}")
    return wanted

def main(argv:list, paras:dict):
    try:
        action = argv[0]
        if action == "help":
            help()
            return 0

        # initialize environment data
        env = collectEnv(paras, argv[3] if len(argv) > 3 else None, argv[2] if len(argv) > 2 else None)
        # if len(argv) > 3: env["platform"] = argv[3]             
        # if len(argv) > 2: env["target"] = argv[2]             
        env["objpath"] = ("{1}" if env["platform"] == "x86" else "{0}/{1}").format(env["platform"], env["target"])
        for t in _AllTargets:
            t.initEnv(env)

        objproj = argv[1] if len(argv) > 1 else None
        projects = ProjectSet.gatherFrom()
        projects.solveDependency()
        ProjectSet.loadSolution(env)

        if action == "test":
            for proj in projects:
                proj.solveTargets(env)
                print(f"{proj.name}\n{str(proj)}\n\n")
                proj.writeMakefile(env)
        elif action == "list":
            listproj(projects, objproj)
            return 0
        elif action in set(["build", "buildall", "clean", "cleanall", "rebuild", "rebuildall"]):
            projs = parseProj(objproj, projects)
            if env["osname"] == "Windows":
                print(COLOR.Yellow("For Windows, use Visual Studio 2019 to build!"))
                return -1
            elif env["osname"] == "Darwin":
                print(COLOR.Yellow("maxOS support is not tested!"))
            elif env["osname"] != "Linux":
                print(COLOR.Yellow("unknown OS!"))
                return -1
            
            suc, tol = mainmake(action, projs, env)
            preclr = COLOR.red if suc == 0 else COLOR.yellow if suc < tol else COLOR.green
            print(f"{preclr}build [{suc}/{tol}] successed.{COLOR.clear}")
            return 0 if suc == tol else -2
        else:
            raise IndexError()
    except IndexError:
        print(f"{COLOR.red}unknown action: {argv}{COLOR.clear}")
        help()
        return -1
    except KeyError:
        print(f"{COLOR.red}cannot find target project [{objproj}]{COLOR.clear}")
        return -1
    pass

args  = [x for x in sys.argv[1:] if not x.startswith("/")]
paras = [x[1:].split("=") for x in sys.argv[1:] if x.startswith("/")]
paras = {p[0]:"=".join(p[1:]) for p in paras}
sys.exit(main(args, paras))
