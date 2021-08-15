import os
import chardet
import codecs
import platform
import re
import sys
from subprocess import call

CurDir = os.path.dirname(__file__)
# enable package import when executed directly
if not __package__:
    path = os.path.join(CurDir, os.pardir)
    sys.path.insert(0, path)

from xzbuild import COLOR
from xzbuild.Environment import findAppInPath

OSName = platform.system()
class Resource:
    def __init__(self, idr:str, fname:str, objdir:str):
        self.idr = idr
        self.fpath = fname
        self.define = "IDR_" + idr
        self.objpath = os.path.join(objdir, fname + ".rc.o")
        self.symbol = fname.replace(".", "_")
        pass
    def __str__(self):
        return "[{}]@[{}]".format(self.define, self.fpath)

BinTypeMap = \
{
    "x86":      "elf_i386",
    "x64":      "elf_x86_64",
    "ARM":      "armelf",
    "ARM64":    "aarch64elf",
}


def findApp(*names) -> str:
    for name in names:
        paths = findAppInPath(name)
        if len(paths) > 0: return name
    print(COLOR.Yellow(f"Seems {names[0]} is not found"))
    return None

if __name__ == "__main__":
    compiler = findApp("c++", "g++", "clang++")

    rcfile = sys.argv[1]
    plat = sys.argv[2]
    objdir = sys.argv[3]
    print(f"{COLOR.green}Compiling Resource File{COLOR.clear} [{COLOR.Magenta(rcfile)}] for [{COLOR.Magenta(plat)}] to [{objdir}]")
    with open(rcfile, "rb") as fp:
        rawdata = fp.read()
    enc = chardet.detect(rawdata)["encoding"]
    txtdata = rawdata.decode(enc)

    #parse binary resources
    ress = []
    incs = []
    for line in txtdata.splitlines():
        mth = re.match(r'IDR_(\w+)\s+\w+\s+\"(.+)\"', line)
        if mth != None:
            ress.append(Resource(mth.group(1), mth.group(2), objdir))
        else:
            mth2 = re.match(r'\#\s*include\s*[\"\<](.+)[\"\>]', line)
            if mth2 != None:
                incs.append(mth2.group(1))

    incs = set(incs)
    incs.difference_update(["winres.h", "windows.h", "Windows.h"])
    cpp = ""
    for inc in incs: 
        cpp += f'#include "{inc}"\r\n'
    cpp += """
#include "RCInclude.h" 
INCHEADER
"""
    reg = """
REGHEADER
"""

    objfpath = os.path.join(objdir, rcfile + '.o')
    cppfpath = os.path.join(objdir, rcfile + ".cpp")
    depfpath = os.path.join(objdir, rcfile + ".d")
    dep = f"{objfpath}: {cppfpath} {os.path.join(CurDir, 'RCInclude.h')} "
   
    for r in ress:
        print(f"[{COLOR.Magenta(r.define)}]@[{COLOR.Cyan(r.fpath)}]")
        cpp += f"INCDATA({r.idr}, \"{r.fpath}\")\r\n"
        reg += f"REGDATA({r.idr})\r\n"
        dep += f" {r.fpath}"
    cpp += "INCFOOTER\r\n"
    reg += "REGFOOTER\r\n"
    cpp += reg

    with open(cppfpath, "w") as fp:
        fp.write(cpp)
    with open(depfpath, "w") as fp:
        fp.write(dep)
    
    #compile rc-cpp
    cmd = f"{compiler} -fPIC -std=c++11 -fvisibility=hidden -I. -I{CurDir} -c {cppfpath} -o {objfpath}"
    print(cmd)
    retcode = call(cmd, shell=True)
    sys.exit(retcode)