#! /bin/python3

from collections.abc import Iterable
from inspect import isfunction
import os
import sys
from collections import OrderedDict, defaultdict
import subprocess
import funcs
import librofiscript 
from librofiscript import SEP

def run_shell_async(shell):
    # shell = f"{shell} > /dev/null 2>&1"
    subprocess.Popen(shell,stdout=subprocess.DEVNULL,shell=True)

def run_shell(shell):
    cmd = subprocess.Popen(shell,stdout=subprocess.PIPE,shell=True)
    output = cmd.stdout.read().decode() if cmd.stdout else None
    return output


def showmenus(cmds):
    for cmd in cmds:
        print(cmd)

def parsepath(cmdpath):
    return cmdpath.split(SEP)[1:]

def getbypath(d, path, default=None):
    ks = parsepath(path)
    if not ks:
        return default
    if not d:
        return default
    result = d 
    for k in ks:
        if not isinstance(result, dict):
            return None
        if k not in result:
            return default
        result = result[k]
    return result

def _sortbyfreq(root, path, exclude_paths = []):
    if not isinstance(root, dict):
        return root
    if path in exclude_paths:
        return root
    newroot = librofiscript.sortbyfreq(root, path)
    for k, v in root.items():
        newroot[k] = _sortbyfreq(v, f"{path}{SEP}{k}", exclude_paths)
    return newroot

cmds = OrderedDict()
# cmds[sys.argv[0]] = {}

for funcname in funcs.__all__:
    eval(f"funcs.{funcname}")(cmds)


freq_exclude_paths = [
    f"{SEP}context",
]

cmds = _sortbyfreq(cmds, "", exclude_paths = freq_exclude_paths)
# print(cmds)

rofiretv = os.environ["ROFI_RETV"]
if rofiretv == "0":
    showmenus(cmds)
    print(f"\0data\x1f\n")
if rofiretv == "1":
    cmd = sys.argv[1]
    lastpath = os.environ["ROFI_DATA"]
    path = lastpath + f"{SEP}{cmd}"
    # print(path)
    print(f"\0data\x1f{path}\n")
    cmditem = getbypath(cmds, path, funcs.emptyfunc)
    if cmditem:
        if isinstance(cmditem, dict):
            showmenus(cmditem)
        if isfunction(cmditem): 
            cmditem(cmd, path, cmds)
        librofiscript.recordfreq(path)

