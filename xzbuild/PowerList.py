from collections import OrderedDict


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
    if not isinstance(element, dict): # single element
        return ([element], [])
    if not all(_checkMatch(element, env, n, t) for n,t in __tests.items()): # pre-check conditions
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

