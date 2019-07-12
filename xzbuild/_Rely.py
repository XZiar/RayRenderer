from collections import OrderedDict

class COLOR:
    black	= "\033[90m"
    red		= "\033[91m"
    green	= "\033[92m"
    yellow	= "\033[93m"
    blue	= "\033[94m"
    magenta	= "\033[95m"
    cyan	= "\033[96m"
    white	= "\033[97m"
    clear	= "\033[39m"
    @staticmethod
    def Black(val):
        return COLOR.black  + str(val) + COLOR.clear
    @staticmethod
    def Red(val):
        return COLOR.red    + str(val) + COLOR.clear
    @staticmethod
    def Green(val):
        return COLOR.green  + str(val) + COLOR.clear
    @staticmethod
    def Yellow(val):
        return COLOR.yellow + str(val) + COLOR.clear
    @staticmethod
    def Blue(val):
        return COLOR.blue   + str(val) + COLOR.clear
    @staticmethod
    def Magenta(val):
        return COLOR.magenta+ str(val) + COLOR.clear
    @staticmethod
    def Cyan(val):
        return COLOR.cyan   + str(val) + COLOR.clear
    @staticmethod
    def White(val):
        return COLOR.white  + str(val) + COLOR.clear


__tests = \
{
    "ifhas": lambda env,x: x in env,
    "ifno" : lambda env,x: x not in env,
    "ifeq" : lambda env,x: env.get(x[0]) == x[1],
    "ifneq": lambda env,x: env.get(x[0]) != x[1],
    "ifin" : lambda env,x: x[1] in env.get(x[0], []),
    "ifnin": lambda env,x: x[1] not in env.get(x[0], []),
}

def _checkMatch(obj:dict, env:dict, name:str, test) -> bool:
    '''given [env] and a [test], check if target corresponding to the [name] in [obj] satisfies'''
    target = obj.get(name)
    if target is None:
        return True
    if isinstance(target, list):
        return all([test(env, t) for t in target])
    if isinstance(target, dict):
        return all([test(env, t) for t in target.items()])
    else:
        return test(target)

def _solveElement(element, env:dict, postproc) -> tuple:
    if isinstance(element, list):
        return (element, [])
    if not isinstance(element, dict):
        return ([element], [])
    if not all(_checkMatch(element, env, n, t) for n,t in __tests.items()):
        return ([], [])
    adds = element.get("+", [])
    adds = adds if isinstance(adds, list) else [adds]
    dels = element.get("-", [])
    dels = dels if isinstance(dels, list) else [dels]
    ret = (adds, dels)
    if not postproc == None:
        ret = postproc(ret, element, env)
    return ret

def solveElementList(target, field:str, env:dict, postproc=None) -> tuple:
    eles = target.get(field, [])
    if not isinstance(eles, list):
        return _solveElement(eles, env, postproc)
    middle = list(_solveElement(ele, env, postproc) for ele in eles)
    adds = list(i for a,_ in middle for i in a)
    dels = list(i for _,d in middle for i in d)
    return (adds, dels)

def combineElements(original, adds, dels):
    added = OrderedDict.fromkeys(original + adds)
    wantDel = set(dels)
    return list(x for x in added.keys() if x not in wantDel)


def writeItems(file, name:str, val:list, state = ":"):
    file.write("{}\t{}= {}\n".format(name, state, " ".join(val)))
def writeItem(file, name:str, val:str, state = ":"):
    file.write("{}\t{}= {}\n".format(name, state, val))
