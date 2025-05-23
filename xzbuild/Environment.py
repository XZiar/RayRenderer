import json
import plistlib
import os
import re
import errno
import platform
import subprocess
import sys
import time
import tempfile

from . import COLOR

_intrinMap = \
{
    "__SSE__":              "sse",
    "__SSE2__":             "sse2",
    "__SSE3__":             "sse3",
    "__SSSE3__":            "ssse3",
    "__SSE4_1__":           "sse41",
    "__SSE4_2__":           "sse42",
    "__AVX__":              "avx",
    "__FMA__":              "fma",
    "__AVX2__":             "avx2",
    "__AVX512F__":          "avx512f",
    "__AVX512VL__":         "avx512vl",
    "__AVX512VNNI__":       "avx512vnni",
    "__F16C__":             "f16c",
    "__BMI__":              "bmi1",
    "__BMI2__":             "bmi2",
    "__AES__":              "aes",
    "__SHA__":              "sha",
    "__PCLMUL__":           "pclmul",
    "__ARM_NEON":           "neon",
    "__ARM_FEATURE_CRC32":  "acle_crc"
}
_intrinAVX31 = ["__AVX512F__", "__AVX512CD__"] # skip ER & PF
_intrinAVX32 = ["__AVX512F__", "__AVX512CD__", "__AVX512BW__", "__AVX512DQ__", "__AVX512VL__"]

_checkCpp = '''
#if __cplusplus >= 202002L
#   include <version>
#elif defined(__has_include)
#   if __has_include(<ciso646>)
#       include <ciso646>
#   else
#       include <iso646.h>
#   endif
#else
#   include <ciso646>
#endif
int main() { }
'''

def _checkExists(prog:str) -> bool:
    FNULL = open(os.devnull, 'w')
    try:
        subprocess.call([prog], stdout=FNULL, stderr=FNULL)
    except OSError as e:
        if e.errno == errno.ENOENT:
            return False
    return True

def _fallbackCheck(paras:dict, paraName:str, envName:str, defProvider, defVal:str, checkExists:bool) -> str:
    check = (lambda x: x is not None and _checkExists(x)) if checkExists else (lambda x: x is not None)

    target = paras.get(paraName, None)
    if check(target): return target

    target = os.environ.get(envName, None)
    if check(target): return target

    if defProvider is not None:
        target = defProvider if isinstance(defProvider, str) else defProvider()
        if check(target): return target

    return defVal

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

def strToVer(verstr:str, count:int) -> int:
    trimedVer = re.match(r"([\d|\.])+", verstr).group(1)
    vers = trimedVer.split(".")
    muler = pow(100, count - 1)
    ver = 0
    for part in vers:
        ver += int(part) * muler
        muler /= 100
    return int(ver)

def loadConfig() -> dict:
    try:
        with open(os.path.join(os.path.expanduser("~"), ".xzbuild.json")) as fp:
            conf = json.load(fp)
            print("System user config loaded.")
            return conf.get("params", {})
    except:
        return {}

def collectEnv(paras:dict, plat:str, tgt:str) -> dict:
    solDir = os.getcwd()
    xzbuildPath = os.path.relpath(os.path.abspath(os.path.dirname(__file__)), solDir)
    env = {"rootDir": solDir, "xzbuildPath": xzbuildPath, "paras": paras}
    env["target" ] = "Debug" if tgt is None else tgt
    env["verbose"] = "verbose" in paras
    env["incDirs"] = []
    env["libDirs"] = []
    env["defines"] = []
    env["incDirs"] += [x+"/include" for x in [os.environ.get("CPP_DEPENDENCY_PATH")] if x is not None]
    
    cppcompiler = _fallbackCheck(paras, "cppcompiler", "CPPCOMPILER", None, "c++", True)
    ccompiler   = _fallbackCheck(paras, "ccompiler",   "CCOMPILER",   None, "cc",  True)
    env["cppcompiler"] = cppcompiler
    env["ccompiler"] = ccompiler
    env["osname"] = platform.system()
    env["machine"] = platform.machine()
    iOSSdkInfo = None
    if env["machine"].startswith("iPhone") or env["machine"].startswith("iPad"):
        env["iOS"] = True
        if "isdkroot" in paras:
            env["isdkroot"] = paras["isdkroot"]
        try:
            with open(os.path.join(paras.get("isdkroot", "/usr/share/SDKs/iPhoneOS.sdk"), "SDKSettings.plist"), "rb") as fp:
                iOSSdkInfo = plistlib.load(fp)
        except FileNotFoundError:
            pass
        if "iOSSDKVer" in paras:
            env["iOSSDKVer"] = paras["iOSSDKVer"]
        elif iOSSdkInfo is not None:
            env["iOSSDKVer"] = strToVer(iOSSdkInfo["Version"], 3) 
    if plat is None:
        is64Bits = sys.maxsize > 2**32
        env["bits"] = 64 if is64Bits else 32
        if env["machine"] in ["i386", "AMD64", "x86", "x86_64"]:
            env["arch"] = "x86"
            env["platform"] = "x64" if is64Bits else "x86"
        elif env["machine"] in ["arm", "aarch32_be", "aarch32", "armv7b", "armv7l", "aarch64_be", "aarch64", "armv8b", "armv8l"] or "iOS" in env:
            env["arch"] = "arm"
            env["platform"] = "ARM64" if is64Bits else "ARM"
        else:
            env["arch"] = ""
            env["platform"] = "x64" if is64Bits else "x86"
    else:
        is64Bits = plat in ["x64", "ARM64"]
        env["bits"] = 64 if is64Bits else 32
        if plat in ["ARM", "ARM64"]:
            env["arch"] = "arm"
        elif plat in ["x64", "x86"]:
            env["arch"] = "x86"
        else:
            env["arch"] = ""
        env["platform"] = plat
    env["objpath"] = ("{1}" if env["platform"] == "x86" else "{0}/{1}").format(env["platform"], env["target"])
    
    targetarch = paras.get("targetarch", "native")
    rawdefs = ""
    defs = {}
    if not env["osname"] == "Windows":
        qarg = "arch" if env["arch"] == "x86" else "cpu"
        env["archparam"] = f"{qarg}={targetarch}"
        env["stdlibarg"] = f"-stdlib={paras['stdlib']}" if "stdlib" in paras else ""
        rawdefs = subprocess.check_output(f"{cppcompiler} -m{env['archparam']} {env['stdlibarg']} -xc++ -dM -E -", shell=True, input=_checkCpp.encode()).decode()
        defs = dict([d.split(" ", 2)[1:3] for d in rawdefs.splitlines()])
        env["libDirs"] += splitPaths(os.environ.get("LD_LIBRARY_PATH"))
        compilerType = "clang" if "__clang__" in defs else "gcc"
        env["compiler"] = compilerType
        env["arlinker"] = _fallbackCheck(paras, "arlinker", "STATICLINKER", "gcc-ar" if compilerType == "gcc" else "llvm-ar", "ar", True)

        if compilerType == "clang":
            env["clangVer"] = int(defs["__clang_major__"]) * 10000 + int(defs["__clang_minor__"]) * 100 + int(defs["__clang_patchlevel__"])
        else:
            env["gccVer"] = int(defs["__GNUC__"]) * 10000 + int(defs["__GNUC_MINOR__"]) * 100 + int(defs["__GNUC_PATCHLEVEL__"])

        if "_LIBCPP_VERSION" in defs:
            env["stdlib"] = "libc++"
            ver = int(defs["_LIBCPP_VERSION"])
            env["libc++Ver"] = int(ver % 1000 + ver / 1000 * 10000)
        elif "__GLIBCXX__" in defs:
            env["stdlib"] = "libstdc++"
            if "_GLIBCXX_RELEASE" in defs: # introduced in 7.1
                env["libstdc++Ver"] = int(defs["_GLIBCXX_RELEASE"]) * 10000
            elif compilerType == "gcc":
                env["libstdc++Ver"] = env["gccVer"]
            else:
                env["libstdc++Ver"] = 60000 # assume 6.0.0
        else:
            env["stdlib"] = ""
        
        if "__BIONIC__" in defs:
            env["libc"] = "bionic"
        elif "__GLIBC__" in defs:
            env["libc"] = "glibc"
            env["glibcVer"] = int(defs["__GLIBC__"]) * 100 + int(defs["__GLIBC_MINOR__"])
    else:
        env["compiler"] = "msvc"

    if "dumpdefine" in paras:
        outfile = os.path.join(tempfile.gettempdir(), f"xzbuild-{env['compiler']}.h")
        with open(outfile, "w") as file:
            file.write(rawdefs)
        print(f"defined output to [{outfile}]")

    env["intrin"] = set(i[1] for i in _intrinMap.items() if i[0] in defs)
    if all(k in defs for k in _intrinAVX31):
        env["intrin"].add("avx31")
    if all(k in defs for k in _intrinAVX32):
        env["intrin"].add("avx32")
    if "__ANDROID__" in defs:
        env["android"] = True
    if "__NDK_MAJOR__" in defs:
        env["ndkVer"] = int(defs["__NDK_MAJOR__"]) * 100 + int(defs.get("__NDK_MINOR__", "0"))
    if env["arch"] == "arm":
        env["armArch"] = int(defs["__ARM_ARCH"])
        env["armArchProfile"] = defs["__ARM_ARCH_PROFILE"].strip("'")
    if "iOS" in env:
        if "iOSSDKVer" not in env:
            envSdkVer = os.environ.get("SDKVERSION")
            if envSdkVer is not None:
                env["iOSSDKVer"] = strToVer(envSdkVer)
            else:
                env["iOSSDKVer"] = int(env.get("__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__", 70000))
        if "iOSVer" in paras:
            env["iOSVer"] = strToVer(paras["iOSVer"], 3)
            if iOSSdkInfo is not None:
                okVals = iOSSdkInfo["DefaultProperties"]["DEPLOYMENT_TARGET_SUGGESTED_VALUES"]
                if paras["iOSVer"] not in okVals:
                    print(COLOR.Yellow(f"Target SDK Version {env['iOSVer']} not within support list {okVals}"))
            elif env["iOSVer"] > env["iOSSDKVer"]:
                print(COLOR.Yellow(f"Target SDK Version {env['iOSVer']} higher than installed SDK {env['iOSSDKVer']}"))
        else:
            env["iOSVer"] = env["iOSSDKVer"]
        print(f"targeting iOS[{env['iOSVer']}] with sdk [{env['iOSSDKVer']}]")

    termuxVer = os.environ.get("TERMUX_VERSION")
    if termuxVer is not None:
        env["termuxVer"] = strToVer(termuxVer, 3) 
        # pkgs = subprocess.check_output("pkg list-installed", shell=True)
        # libcxx = [x for x in pkgs.decode().splitlines() if x.startswith("libc++")][0]
        # libcxxVer = libcxx.split(",")[1].split(" ")[1].split("-")[0]
        # env["ndkVer"] = int(libcxxVer[:-1]) * 100 + (ord(libcxxVer[-1]) - ord("a"))
    
    env["cpuCount"] = os.cpu_count()
    env["gprof"] = "gprof" in paras
    threads = paras.get("threads", "x1")
    if threads.startswith('x'):
        env["threads"] = int(env["cpuCount"] * float(threads[1:]))
    else:
        env["threads"] = int(threads)
    if "dbgsym" in paras:
        env["dsym"] = paras["dbgsym"]
    if "dsymlv" in paras:
        env["dslv"] = paras["dsymlv"]

    for k,v in paras.items():
        if k.startswith("set:"):
            env[k[4:]] = v

    return env

def writeEnv(env:dict):
    os.makedirs("./" + env["objpath"], exist_ok=True)
    with open(os.path.join(env["objpath"], "xzbuild.sol.mk"), 'w') as file:
        file.write("# xzbuild per solution file\n")
        file.write("# written at [{}]\n".format(time.asctime(time.localtime(time.time()))))
        for k,v in env.items():
            if isinstance(v, (str, int)):
                file.write("xz_{}\t = {}\n".format(k, v))
            #else:
            #    print(f"skip {k}")
        file.write("xz_incDir\t = {}\n".format(" ".join(env["incDirs"])))
        file.write("xz_libDir\t = {}\n".format(" ".join(env["libDirs"])))
        file.write("xz_define\t = {}\n".format(" ".join(env["defines"])))
        file.write("STATICLINKER\t = {}\n".format(env["arlinker"]))