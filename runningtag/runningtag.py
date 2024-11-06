#!/bin/python3

import json
from os import kill
import time
import subprocess
import json

from datetime import datetime
import sched
import re
import signal

def run_shell_async(shell):
    # shell = f"{shell} > /dev/null 2>&1"
    subprocess.Popen(shell,stdout=subprocess.DEVNULL,shell=True)

def run_shell(shell):
    cmd = subprocess.Popen(shell,stdout=subprocess.PIPE,shell=True)
    output = cmd.stdout.read().decode() if cmd.stdout else None
    return output

def get_dwm_clients():
    cmd = "dwm-msg get_dwm_clients"
    output = run_shell(cmd)
    return json.loads(output) if output else None

def change_window_property(win, name, value):
    cmd = f"xprop -id {win} -f {name} 8u -set {name} '{value}'"
    run_shell_async(cmd)

def isrunning(client):
    pid = client["pid"]
    if client["class"] != "St":
        return False
    output= run_shell(f"pstree {pid}")
    if output and "st---zsh-" in output:
        return True
    return False

def getrunningcmd(ppid):
    pid = 0
    output= run_shell(f"pstree -p {ppid}")
    if output:
        groups = re.search(r"st\(\d+\)---zsh\(\d+\)---.*?\((\d+)\).*", output)
        pid = int(groups[1]) if groups else 0
    cmd = run_shell(f"ps -p {pid} -o args=")
    if cmd:
        cmd = cmd.replace("/bin/bash", "").strip()
        cmd =  re.sub(r"^/.*/(\w+) ", r"\1 ",cmd)
        cmd = cmd.replace("python3 ", "py ")
        return cmd

def func():
    cs = get_dwm_clients()
    if cs:
        for c in cs:
            oldname = c["name"]
            name = c["name"]
            if isrunning(c):
                # change_window_property(c["window_id"], "_NET_MY_NOTE", "running")
                while name.startswith(MARK):
                    name = name[1:]
                name = f"{MARK}{name}"
                if name != oldname:
                    change_window_property(c["window_id"], "_NET_WM_NAME", name)
                    change_window_property(c["window_id"], "WM_NAME", name)
                    runningcmd = getrunningcmd(c["pid"])
                    if runningcmd:
                        change_window_property(c["window_id"], "_NET_MY_NOTE", runningcmd)
            else:
                while name.startswith(MARK):
                    name = name[1:]
                if name != oldname:
                    change_window_property(c["window_id"], "_NET_WM_NAME", name)
                    change_window_property(c["window_id"], "WM_NAME", name)
                    change_window_property(c["window_id"], "_NET_MY_NOTE", "")
        
        for c in cs:
            lastfocusduration = c["stastic"]["lastunfocustime"] - c["stastic"]["lastfocustime"]
            if c["class"] == "St" \
                and c["stastic"]["lastfocustime"] < (time.time() - 60*60) * 10**6 \
                and not c["states"]["is_focused"] \
                and not isrunning(c):
                kill(c["pid"], signal.SIGTERM)

def schedule(func, delay):
    def wrapfunc():
        func()
        scheduler.enter(delay, 1, wrapfunc)
    # 初始化 sched 模块的 scheduler 类
    scheduler = sched.scheduler()
    # 增加调度任务
    scheduler.enter(delay, 1, wrapfunc)
    # 运行任务
    scheduler.run()

if __name__ == '__main__':
    MARK = "●"
    schedule(func, 1)
