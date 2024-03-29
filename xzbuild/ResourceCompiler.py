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


if __name__ == "__main__":

    rcfile = sys.argv[1]
    rcDir = os.path.dirname(rcfile)
    plat = sys.argv[2]
    objdir = sys.argv[3]
    compiler = sys.argv[4]
    common = os.path.relpath(os.path.commonpath([os.path.abspath(rcfile), os.path.abspath(os.path.curdir)]))
    rcrelfile = os.path.relpath(rcfile, common)
    print(f"{COLOR.green}Compiling Resource File{COLOR.clear} [{COLOR.Magenta(rcfile)}] for [{COLOR.Magenta(plat)}] to [{objdir}] as [{rcrelfile}.o]")
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
    objfpath = os.path.join(objdir, rcrelfile + '.o')
    cppfpath = os.path.join(objdir, rcrelfile + ".cpp")
    depfpath = os.path.join(objdir, rcrelfile + ".d")
    dep = f"{objfpath}: {cppfpath} {os.path.join(CurDir, 'RCInclude.h')} "
   
    for r in ress:
        print(f"[{COLOR.Magenta(r.define)}]@[{COLOR.Cyan(r.fpath)}]")
        cpp += f"INCDATA({r.idr}, \"{r.fpath}\")\r\n"
        reg += f"REGDATA({r.idr})\r\n"
        frelpath = os.path.normpath(os.path.join(rcDir, r.fpath))
        dep += f" {frelpath}"
    cpp += "INCFOOTER\r\n"
    reg += "REGFOOTER\r\n"
    cpp += reg

    with open(cppfpath, "w") as fp:
        fp.write(cpp)
    with open(depfpath, "w") as fp:
        fp.write(dep)
    
    #compile rc-cpp
    rcInclude = f"-I. -I{rcDir}" if rcDir else "-I."
    cmd = f"{compiler} -fPIC -std=c++11 -fvisibility=hidden {rcInclude} -I{CurDir} -c {cppfpath} -o {objfpath}"
    print(cmd)
    retcode = call(cmd, shell=True)
    sys.exit(retcode)