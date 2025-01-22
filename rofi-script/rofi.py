#! /bin/python3
from inspect import isfunction
import os
import sys
import subprocess
from typing import Optional
import funcs
from librofiscript import SEP, Node, sortbyfreq, recordfreq


def run_shell_async(shell):
    # shell = f"{shell} > /dev/null 2>&1"
    subprocess.Popen(shell,stdout=subprocess.DEVNULL,shell=True)

def run_shell(shell):
    cmd = subprocess.Popen(shell,stdout=subprocess.PIPE,shell=True)
    output = cmd.stdout.read().decode() if cmd.stdout else None
    return output


def showmenus(cmds:dict):
    for cmd in cmds.values():
        print(cmd.value)

def parsepath(cmdpath):
    return cmdpath.split(SEP)[1:]

def getbypath(d, path, default=None) -> Optional[Node]:
    ks = parsepath(path)
    if not ks:
        return default
    if not d:
        return default
    result = d 
    for k in ks:
        if not isinstance(result, Node):
            return None
        if k not in result:
            return default
        result = result[k]
    return result

def _sortbyfreq(root:Node, path, exclude_paths = []):
    if not isinstance(root, dict):
        return root
    if path in exclude_paths:
        return root
    if not root.usefreq:
        return root
    newroot = sortbyfreq(root, path)
    for k, v in root.items():
        newroot[k] = _sortbyfreq(v, f"{path}{SEP}{k}", exclude_paths)
    return newroot

def initpathmenu(path, cmds):
    # 把上级全都执行一遍, 防止懒加载加载不到的情况
    tmppathlist = []
    for name in path.split(SEP)[1:]:
        tmppathlist.append(name)
        tmppath = SEP + SEP.join(tmppathlist)
        cmditemdir = getbypath(cmds, tmppath, None)
        if cmditemdir is not None and cmditemdir.func:
            cmditemdir.func(name, tmppath, cmds)


# cmds = OrderedDict()
cmds = Node()
# cmds.usefreq = False
# cmds[sys.argv[0]] = {}

for funcname in funcs.__all__:
    eval(f"funcs.{funcname}")(cmds)

freq_exclude_paths = [
    f"{SEP}context",
]

rofidata = os.environ.get("ROFI_DATA")
if rofidata:
    subcmds = getbypath(cmds, rofidata.replace("/", SEP))
    if subcmds is not None:
        subcmds.forcetop = 9999 

cmds = _sortbyfreq(cmds, "", exclude_paths = freq_exclude_paths)

# 添加tops
# topcmds = Node()
# topcmds[f"clipboard"] = cmds[f"clipboard"]
# clipliststr = run_shell("clipcatctl list")
# if clipliststr:
#     groups = re.findall("([a-zA-Z0-9]{16}): (.*)", clipliststr)
#     if groups:
#         abstract2id = {}
#         for group in groups:
#             abstract2id[group[1]] = group[0]
#         for group in groups[:5]:
#             abstract = group[1]
#             def _(arg, path, rofi):
#                 id = abstract2id[arg]
#                 content = run_shell(f"clipcatctl get {id}")
#                 if content:
#                     # 这里容易被误杀
#                     copy(content.replace("\\n", "\n"))
#             add([abstract], _, topcmds)

# 合并
# newcmds = Node()
# for k, v in topcmds.items():
#     newcmds[k] = v
# for k, v in cmds.items():
#     newcmds[k] = v
# cmds = newcmds

rofiretv = os.environ["ROFI_RETV"]
if rofiretv == "0":
    if rofidata:
        # 传入环境变量, 指定打开的路径
        path = rofidata.replace("/", SEP)
        initpathmenu(path, cmds)
        subcmd = getbypath(cmds, path)
        if subcmd is not None:
            subcmd.forcetop = 9999 
            if subcmd:
                showmenus(subcmd)
                print(f"\0data\x1f{path}\n")
            elif subcmd.func and isfunction(subcmd.func): 
                subcmd.func(None, path, cmds)
    else:
        showmenus(cmds)
        print(f"\0data\x1f\n")
if rofiretv == "1":
    cmd = sys.argv[1]
    lastpath = os.environ["ROFI_DATA"]
    path = lastpath + f"{SEP}{cmd}"
    # print(path)
    print(f"\0data\x1f{path}\n")
    initpathmenu(path, cmds)
    cmditem = getbypath(cmds, path)
    if cmditem:
        showmenus(cmditem)
    recordfreq(path)

