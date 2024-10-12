from collections import OrderedDict


__tests = \
{
    "ifhas": lambda env,x: x in env,
    "ifno" : lambda env,x: x not in env,
    "ifeq" : lambda env,x: env.get(x[0]) == x[1],
    "ifneq": lambda env,x: env.get(x[0]) != x[1],
    "ifin" : lambda env,x: x[1] in env.get(x[0], []),
    "ifnin": lambda env,x: x[1] not in env.get(x[0], []),
    "ifgt" : lambda env,x: env.get(x[0], 0) >  x[1],
    "ifge" : lambda env,x: env.get(x[0], 0) >= x[1],
    "iflt" : lambda env,x: env.get(x[0], 0) <  x[1],
    "ifle" : lambda env,x: env.get(x[0], 0) <= x[1],
}

def _checkMatch(key:str, value, env:dict) -> bool:
    '''given [env] and a [test], check if target corresponding to the [name] in [obj] satisfies'''
    test = __tests.get(key)
    if test is None:
        return True
    if isinstance(value, list):
        return all([test(env, item) for item in value])
    if isinstance(value, dict):
        return all([test(env, item) for item in value.items()])
    else:
        return test(env, value)

def _solveElement(element, env:dict, postproc) -> tuple:
    if isinstance(element, list):
        return (element, [])
    if not isinstance(element, dict): # single element
        return ([element], [])
    if not all(_checkMatch(k, v, env) for k,v in element.items()): # pre-check conditions
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
    eles = target if field is None else target.get(field, [])
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

def solveSingleElement(target, field:str, env:dict, defaultVal) -> any:
    (adds, dels) = solveElementList(target, field, env)
    if len(dels) > 0: raise Exception(f'expect no "-" for {field}')
    if len(adds) == 0: return defaultVal
    trimAdds = set(adds)
    if len(trimAdds) > 1: raise Exception(f'get multiple result for {field}: {trimAdds}')
    return list(trimAdds)[0]
    
