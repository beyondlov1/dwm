__all__ = [
    "addclipboard"
]


from librofiscript import *
import re


def emptyfunc(arg, path, cmds):
    pass

def copyfunc(arg, path, cmds):
    copy(arg)
    # print(arg)

def addclipboard(cmds):
    def _showclipboards(arg,path,cmds):
        # run_shell_async("nohup clipcat-menu > /dev/null & disown")
        run_shell_async("sleep 0.1 && /bin/clipcat-menu")
    add(["clipboard",], _showclipboards, cmds)

def getcliphistory():
    r = run_shell("""clipcatctl list""")
    if not r:
        return
    clips =  [item.strip() for item in re.split(r"[\n]?[0-9a-zA-Z]{16}.*: ", r)] 
    return clips

# def addclipboard(cmds):
    # """废弃, 因为不能显示回车, 改用showclipboards"""
#     cliphistory = getcliphistory()
#     if not cliphistory:
#         return 
#     for chitem in cliphistory:
#         add(["clipboard", chitem], copyfunc, cmds)


