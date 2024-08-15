__all__ = [
    "showclipboards"
]


from librofiscript import *


def emptyfunc(arg, path):
    pass

def copyfunc(arg, path):
    copy(arg)

def showclipboards():
    def _showclipboards(arg,path):
        run_shell_async("clipcat-menu")
    return _showclipboards, ["clipboard"]

def getcliphistory():
    """废弃, 因为不能显示回车, 改用showclipboards"""
    r = run_shell("""clipcatctl list | awk -F: '{print $2}' | xargs -i echo {}""")
    if not r:
        return [] 
    return [item.strip() for item in r.split("\n")] 


