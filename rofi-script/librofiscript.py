import subprocess
from collections import OrderedDict, defaultdict
import os
import json

SEP = "|$|$|"

def run_shell_async(shell):
    # shell = f"{shell} > /dev/null 2>&1"
    subprocess.Popen(shell,stdout=subprocess.DEVNULL,shell=True)

def run_shell(shell):
    cmd = subprocess.Popen(shell,stdout=subprocess.PIPE,shell=True)
    output = cmd.stdout.read().decode() if cmd.stdout else None
    return output

def copy(text):
    run_shell_async(f"echo -n '{text}' | xclip -selection clipboard")

def getprimary():
    primary = run_shell(f"xclip -selection primary -o")
    if not primary:
        return ""
    return primary

def getclipboard():
    clipboard = run_shell(f"xclip -selection clipboard -o")
    if not clipboard:
        return ""
    return clipboard


class Node(OrderedDict):
    def __init__(self) -> None:
        self.usefreq = True 
        self.forcetop = False

def add(lpath: list, func, d, usefreq = True, forcetop=False):
    tmproot = d 
    for c in lpath[:-1]:
        if c not in tmproot or not isinstance(tmproot[c],dict):
            tmproot[c] = Node()
            tmproot[c].usefreq = usefreq
            tmproot[c].forcetop = forcetop
        tmproot = tmproot[c]
    tmproot[lpath[-1]] = func

def readfile(path):
    if not os.path.exists(path):
        return None
    with open(path, "r") as f:
        return f.read()

def writefile(path, data):
    with open(path, "w+") as f:
        f.write(data)

def loadjsonfile(path):
    if not os.path.exists(path):
        return None
    with open(path, "r") as f:
        return json.load(f)

def writejsonfile(path, data):
    with open(path, "w+") as f:
        json.dump(data, f)

freqpath = "/tmp/rofi-script.freq"
def recordfreq(path):
    freqdict = loadjsonfile(freqpath)
    freqdict = defaultdict(int) if not freqdict else defaultdict(int, freqdict)
    freqdict[path] += 1
    writejsonfile(freqpath, freqdict)
    
def sortbyfreq(d: dict, path):
    freqdict = loadjsonfile(freqpath)
    freqdict = defaultdict(int) if not freqdict else defaultdict(int, freqdict)
    # print(freqdict)
    sitems = [{"freq": freqdict[f"{path}{SEP}{k}"], "k": k, "v": v, "path": f"{path}{SEP}{k}"} for k,v in d.items()]
    sitems.sort(key = lambda x: x["freq"], reverse = True)
    # print(sitems)
    result = Node()
    for sitem in sitems:
        if isinstance(sitem["v"], Node) and sitem["v"].forcetop:
            result[sitem["k"]] = d[sitem["k"]]
    for sitem in sitems:
        result[sitem["k"]] = d[sitem["k"]]
    return result











# def recordfreq(path):
#     p = multiprocessing.Process(target=_recordfreq, args=(path,))
#     p.start()
#     p.join()

# def sortbyfreq(d):
#     q = multiprocessing.Queue(maxsize=1)
#     p = multiprocessing.Process(target=_sortbyfreq, args=(d,q))
#     p.start()
#     r = q.get()
#     p.join()
#     return r
