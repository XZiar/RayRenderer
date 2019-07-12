import json
import os
import platform
import subprocess
import sys
import time


_intrinMap = \
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


def collectEnv(paras:dict) -> dict:
    solDir = os.getcwd()
    env = {"rootDir": solDir, "target": "Debug"}
    is64Bits = sys.maxsize > 2**32
    env["platform"] = "x64" if is64Bits else "x86"
    env["incDirs"] = []
    env["libDirs"] = []
    env["incDirs"] += [x+"/include" for x in [os.environ.get("CPP_DEPENDENCY_PATH")] if x is not None]
    cppcompiler = os.environ.get("CPPCOMPILER", "g++")
    defs = []
    osname = platform.system()
    if not osname == "Windows":
        rawdefs = subprocess.check_output("{} -march=native -dM -E - < /dev/null".format(cppcompiler), shell=True)
        defs = set([d.split()[1] for d in rawdefs.decode().splitlines()])
        env["libDirs"] += splitPaths(os.environ.get("LD_LIBRARY_PATH"))
    env["intrin"] = set(i[1] for i in _intrinMap.items() if i[0] in defs)
    env["compiler"] = "clang" if "__clang__" in defs else "gcc"
    env["cpuCount"] = os.cpu_count()
    env["threads"] = paras.get("threads", env["cpuCount"])
    return env

def writeEnv(env:dict):
    os.makedirs("./" + env["objpath"], exist_ok=True)
    with open(os.path.join(env["objpath"], "xzbuild.sol.mk"), 'w') as file:
        file.write("# xzbuild per solution file\n")
        file.write("# written at [{}]\n".format(time.asctime(time.localtime(time.time()))))
        for k,v in env.items():
            if isinstance(v, str):
                file.write("xz_{}\t = {}\n".format(k, v))
        file.write("xz_incDir\t = {}\n".format(" ".join(env["incDirs"])))
        file.write("xz_libDir\t = {}\n".format(" ".join(env["libDirs"])))

def splitPaths(path:str):
    if path == None:
        return []
    return [p for p in path.split(os.pathsep) if p]

def findFileInPath(fname:str):
    paths = splitPaths(os.environ["PATH"])
    return [p for p in paths if os.path.isfile(os.path.join(p, fname))]
    #return next((p for p in paths if os.path.isfile(os.path.join(p, fname))), None)

def findAppInPath(appname:str):
    osname = platform.system()
    return findFileInPath(appname+".exe" if osname == "Windows" else appname)

