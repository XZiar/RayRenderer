import os
import chardet
import codecs
import platform
import re
import sys
from subprocess import call

# enable package import when executed directly
if not __package__:
    path = os.path.join(os.path.dirname(__file__), os.pardir)
    sys.path.insert(0, path)

from xzbuild import COLOR
from xzbuild.Environment import findAppInPath

OSName = platform.system()
class Resource:
    def __init__(self, idr:str, fname:str, objdir:str):
        self.fpath = fname
        self.define = "IDR_"+idr
        self.objpath = os.path.join(objdir, fname+".rc.o")
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
    compiler = findApp("c++", "g++")
    ld = findApp("ld", "llvm-ld")

    rcfile = sys.argv[1]
    platform = sys.argv[2]
    objdir = sys.argv[3]
    print(f"{COLOR.green}Compiling Resource File {COLOR.magenta}[{rcfile}]{COLOR.clear} for {COLOR.magenta}[{platform}]{COLOR.clear} to [{objdir}]")
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
}
    """
    """
    #define EXTLD(NAME) \ 
  extern const unsigned char _section$__DATA__ ## NAME []; 
#define LDVAR(NAME) _section$__DATA__ ## NAME 
#define LDLEN(NAME) (getsectbyname("__DATA", "__" #NAME)->size) 
    """
    objfpath = os.path.join(objdir, rcfile + '.o')
    cppfpath = os.path.join(objdir, rcfile + ".cpp")
    depfpath = os.path.join(objdir, rcfile + ".d")
    dep = f"{objfpath}: {cppfpath}"
    for inc in incs: 
        cpp += f'#include "{inc}"\r\n'
    if OSName == "Darwin":
        cpp += """
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>
#include <dlfcn.h>

static const char DummyVal = 0;
static uint32_t RegistResource(const int32_t id, const char* name)
{
    static const auto dllHeader = []() -> const mach_header_64*
    {
        Dl_info info;
        if (dladdr(&DummyVal, &info))
            return reinterpret_cast<const mach_header_64*>(info.dli_fbase);
        return nullptr;
    }();
    if (dllHeader != nullptr)
    {
        unsigned long size = 0;
        const auto ptr = reinterpret_cast<const char*>(getsectiondata(dllHeader, "__DATA", name, &size));
        return common::detail::RegistResource(id, ptr, ptr + size);
    }
    return 0;
}
        """
    for r in ress:
        print(f"[{COLOR.Magenta(r.define)}]@[{COLOR.Cyan(r.fpath)}]")
        if OSName == "Darwin":
            cpp += """
static uint32_t DUMMY_{0} = RegistResource({0}, "__{1}");
            """.format(r.define, r.symbol)
        else:
            cpp += """
extern char _binary_{1}_start, _binary_{1}_end;
static uint32_t DUMMY_{0} = common::detail::RegistResource({0}, &_binary_{1}_start, &_binary_{1}_end);
            """.format(r.define, r.symbol)
        dep += ' {}'.format(r.fpath)
    with open(cppfpath, "w") as fp:
        fp.write(cpp)
    with open(depfpath, "w") as fp:
        fp.write(dep)
    
    #compile rc-cpp
    cmd = f"{compiler} -fPIC -std=c++11 -fvisibility=hidden -I. -c {cppfpath}"
    if OSName == "Darwin":
        #gcc -sectcreate __DATA __example_jpg example.jpg -o myapp myapp.c
        for r in ress:
            cmd += f" -Wl,-sectcreate,__DATA,__{r.symbol},{r.fpath}"
        cmd += f" -o {objfpath}"
    else:
        cmd += f" -o {cppfpath}.o"
    print(cmd)
    retcode = call(cmd, shell=True)

    if not OSName == "Darwin":
        resFiles = " ".join([r.fpath for r in ress])
        #objType = BinTypeMap.get(platform)
        cmd = f"{ld} -r {cppfpath}.o -b binary {resFiles} -o {objfpath}"
        print(cmd)
        retcode = call(cmd, shell=True)

    sys.exit(retcode)