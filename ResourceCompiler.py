import os
import chardet
import codecs
import platform
import re
import sys
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

class Resource:
    def __init__(self, idr:str, fname:str, objdir:str):
        self.fpath = fname
        self.define = "IDR_"+idr
        self.objpath = os.path.join(objdir, fname+".rc.o")
        self.symbol = fname.replace(".", "_")
        pass
    def __str__(self):
        return "[{}]@[{}]".format(self.define, self.fpath)

if __name__ == "__main__":
    rcfile = sys.argv[1]
    platform = sys.argv[2]
    objdir = sys.argv[3]
    elfType = "elf32-i386" if platform == "x86" else "elf64-x86-64" if platform == "x64" else None
    binType = "i386" if platform == "x86" else "i386:x86-64" if platform == "x64" else None
    print("{clr.green}Compiling Resource File {clr.magenta}[{}]{clr.clear} for {clr.magenta}[{}]{clr.clear} to [{}]".format(rcfile, platform, objdir, clr=COLOR))
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
    cpp = """
    #include<cstdint>
    namespace common
    {
        namespace detail
        {
            uint32_t RegistResource(const int32_t id, const char* ptrBegin, const char* ptrEnd);
        }
    }"""
    dep = "{}:".format(os.path.join(objdir, rcfile+".o"))
    for inc in incs: 
        cpp += '#include "{}"\r\n'.format(inc)
    uid = 0
    for r in ress:
        print(r)
        cmd = "objcopy -I binary -O {} -B {} {} {}".format(elfType, binType, r.fpath, r.objpath)
        print(cmd)
        retcode = call(cmd, shell=True)
        cpp += """
        extern char _binary_{1}_start, _binary_{1}_end;
        static uint32_t DUMMY_{0} = common::detail::RegistResource({0}, &_binary_{1}_start, &_binary_{1}_end);
        """.format(r.define, r.symbol)
        dep += ' {}'.format(r.fpath)
    #compile rc-cpp
    cppfpath = os.path.join(objdir, rcfile+".cpp")
    with open(cppfpath, "w") as fp:
        fp.write(cpp)
    with open(os.path.join(objdir, rcfile+".d"), "w") as fp:
        fp.write(dep)
    cmd = "g++ -fPIC -std=c++11 -I. -c {0} -o {0}.o".format(cppfpath)
    print(cmd)
    retcode = call(cmd, shell=True)
    objs = " ".join([r.objpath for r in ress])
    cmd = "ld -r {}.o {} -o {}".format(cppfpath, objs, os.path.join(objdir, rcfile+".o"))
    print(cmd)
    retcode = call(cmd, shell=True)
    pass