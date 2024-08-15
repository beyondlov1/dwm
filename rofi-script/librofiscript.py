import subprocess
from collections import OrderedDict

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

def add(lpath: list, func, d):
    tmproot = d 
    for c in lpath[:-1]:
        if c not in tmproot or not isinstance(tmproot[c],dict):
            tmproot[c] = OrderedDict()
        tmproot = tmproot[c]
    tmproot[lpath[-1]] = func

