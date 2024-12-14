__all__ = [
    # "addclipboard",
    "addoptions",
]


from librofiscript import *
import re
import random
from urllib.parse import quote

def timestamp2datestr(timestamp):
    import datetime
    dt_object = datetime.datetime.fromtimestamp(timestamp)
    formatted_time = dt_object.strftime("%Y-%m-%d %H:%M:%S")
    return formatted_time

def timestamp2datestr2(timestamp):
    import time
    timeArray = time.localtime(timestamp)
    formatted_time = time.strftime("%Y-%m-%d %H:%M:%S", timeArray)
    return formatted_time

def datestr2timestamp(time_string):
    import datetime
    time_format = "%Y-%m-%d %H:%M:%S"
    local_time = datetime.datetime.strptime(time_string, time_format)
    timestamp = local_time.timestamp()
    return int(timestamp) 

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

def _fetchssr():
    import requests
    from bs4 import BeautifulSoup
    cookies = {
        '_gh_sess': 'Jp2aklgPk3XxOWkDE0zf%2Fc7O0vByJ2ijo27Haa%2BroTHSYpfVIB%2FQU8VbSdfoQUMKWum47TCDmKTiQoz7tY7tDNxUMLjv8x1DSVh5VaB3Za5b66vUlyNiXOyEf7krJwjnPxQ0pJ9J%2Bc6zhNaoXZYROrm3u6BuBEURSwwsCs8QQ4NhJxZt7lHShUB87B%2BmGkqpsiYzFlBUpz52WlrB1CGXCjUnSo%2FY9eOfVlmEp%2FN6fDTlMaXHhs%2BO%2F8CBNI1i65dae6LCLkGPx5jED98eE77UUg%3D%3D--VGwvcUp1zdLrN%2BMX--zTK8pZ1d%2B5gagmfVzMU9XA%3D%3D',
        'preferred_color_mode': 'light',
        'tz': 'Asia%2FShanghai',
        '_octo': 'GH1.1.2080720183.1721540339',
    }

    headers = {
        'accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7',
        'accept-language': 'en-US,en;q=0.9',
        'cache-control': 'max-age=0',
        # Requests sorts cookies= alphabetically
        # 'cookie': '_gh_sess=Jp2aklgPk3XxOWkDE0zf%2Fc7O0vByJ2ijo27Haa%2BroTHSYpfVIB%2FQU8VbSdfoQUMKWum47TCDmKTiQoz7tY7tDNxUMLjv8x1DSVh5VaB3Za5b66vUlyNiXOyEf7krJwjnPxQ0pJ9J%2Bc6zhNaoXZYROrm3u6BuBEURSwwsCs8QQ4NhJxZt7lHShUB87B%2BmGkqpsiYzFlBUpz52WlrB1CGXCjUnSo%2FY9eOfVlmEp%2FN6fDTlMaXHhs%2BO%2F8CBNI1i65dae6LCLkGPx5jED98eE77UUg%3D%3D--VGwvcUp1zdLrN%2BMX--zTK8pZ1d%2B5gagmfVzMU9XA%3D%3D; preferred_color_mode=light; tz=Asia%2FShanghai; _octo=GH1.1.2080720183.1721540339',
        'priority': 'u=0, i',
        'sec-ch-ua': '"Google Chrome";v="126", "Chromium";v="126", "Not=A?Brand";v="24"',
        'sec-ch-ua-mobile': '?0',
        'sec-ch-ua-platform': '"Windows"',
        'upgrade-insecure-requests': '1',
        'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64; ) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.6478.61 Chrome/126.0.6478.61 Not/A)Brand/8  Safari/537.36',
    }

    resp = requests.get('https://github.hscsec.cn/Alvin9999/new-pac/wiki/ss%E5%85%8D%E8%B4%B9%E8%B4%A6%E5%8F%B7', cookies=cookies, headers=headers)
    html = resp.text
    soup = BeautifulSoup(html, 'html.parser')
    pres = soup.find_all("pre")
    for pre in pres:
        # print(pre.get_text())
        t = pre.get_text().strip()
        if t.startswith("ssr"):
            return t
    
# def my_dfcondition():
#     # clip = pyperclip.paste()
#     # clip = getprimary()
#     cmd = re.split(r" +", cmdstr)
#     dfstr = ""
#     attrstr = ""
#     if len(cmd) > 1:
#         dfstr = cmd[1]
#     if len(cmd) > 2:
#         attrstr = cmd[2]
#     r = f"tdf = {dfstr}[{dfstr}[\"{attrstr}\"] == ]"
#     return r


def browseropen(url):
    run_shell_async(f"sleep 0.1 && /home/beyond/software/ba {url}")

def simplepageshow(s):
    path = "/tmp/simplepage_content.tmp"
    writefile(path, f"{s}")
    run_shell_async(f"sleep 0.1 && dwm-msg run_command nextmanagetype 2 && /home/beyond/software/browserclip.sh file://{path}")

def stopen(dirpath):
    run_shell_async(f"st -d {dirpath}")

def regex_topidentifyfunc(regex):
    def f():
        clip = getclipboard()
        if re.search(regex, clip.strip()):
            return 9999
        clip = getprimary()
        if re.search(regex, clip.strip()):
            return 9999
        return 0 
    return f

def addoptions(cmds):
    """
    add 方法: 将 path, callback 加入到cmds
    arg: 选中的条目
    path: 菜单路径
    """
    # def _(arg, path, rofi):
    #     arg = arg.replace("|||", "\n")
    #     copy(arg)
    # ls = readfile("/tmp/recent_context.txt")
    # if ls:
    #     for l in reversed(ls.split("\n")[-2:]):
    #         add([f"{l}"], _, cmds)

    def _showclipboards(arg,path,cmds):
        run_shell_async("sleep 0.1 && /bin/clipcat-menu")
    add(["clipboard",], _showclipboards, cmds, forcetop=1)

    def _(arg, path, rofi):
        clip = getclipboard()
        r = clip.lower()
        copy(r)
    add(["tolower",], _, cmds)

    def _(arg, path, rofi):
        url = readfile("/tmp/chrominum_active_url")
        if not url or "github.com" not in url:
            clip = getclipboard()
            url = clip
        if url and "github.com" in url:
            replacetarget = "git.homegu.com"
            r = url.replace("github.com", replacetarget)
            # copy(r)
            run_shell_async(f"sleep 0.1 && /home/beyond/software/ba {r}")
    add(["togithubmirror",], _, cmds, topidentifyfunc=regex_topidentifyfunc(r"http.*github\.com.*"))

    def _(arg, path, rofi):
        r = _fetchssr()
        copy(r)
    add(["fetchssr",], _, cmds)

    def _(arg, path, rofi):
        # r = "https://chat.tinycms.xyz:3002/#/"
        r = "https://freegpt.tech/"
        browseropen(r)
    add(["gpt",], _, cmds)

    def _(arg, path, rofi):
        # r = "https://lite.icoding.ink/#/chat"
        # r = "https://jk92fd.aitianhu1.top/#/chat/1002"
        r = "https://www.yeschat.ai/zh-CN/gpt-4o"
        browseropen(r)
    add(["gpt2",], _, cmds)

    def _(arg, path, rofi):
        # r = "https://dkzpz9.aitianhu1.top/#/chat/1002"
        r = "https://origin.eqing.tech/"
        browseropen(r)
    add(["gpt3",], _, cmds)

    def _(arg, path, rofi):
        r = arg 
        browseropen(r)
    gptmirroritems = loadjsonfile("/tmp/gptmirror.json")
    if gptmirroritems:
        for item in gptmirroritems:
            add(["gptd", item["href"]], _, cmds)

    def _(arg, path, rofi):
        r = "https://www.linshi-email.com/"
        browseropen(r)
    add(["linshiyouxiang_email",], _, cmds)

    def _(arg, path, rofi):
        r = "https://fcp7.com/free-chatgpt-mirroring-sites-list.html"
        browseropen(r)
    add(["gptmirrorsite",], _, cmds)

    def _(arg, path, rofi):
        r = "https://g.savalone.com/"
        browseropen(r)
    add(["google",], _, cmds)

    def _(arg, path, rofi):
        r = "https://fcp7.com/use-google-search.html"
        browseropen(r)
    add(["googlemirrorsite",], _, cmds)

    def _(arg, path, rofi):
        r = "https://radio5.cn/level/net"
        browseropen(r)
    add(["music",], _, cmds)

    def _(arg, path, rofi):
        stopen("/home/beyond/github/dwm")
    add(["dwm",], _, cmds)

    def _(arg, path, rofi):
        stopen("/home/beyond/github/dwm")
        copy("vi rofi-script/funcs.py && sudo -E make clean install")
        run_shell_async("sleep 1 && xdotool key 'shift+Insert'")
    add(["rofiscriptmodify",], _, cmds)

    def _(arg, path, rofi):
        run_shell_async("sleep 0.1 && export PATH=/home/beyond/software:$PATH &&rofi -show run")
    add(["run",], _, cmds)

    def _(arg, path, rofi):
        stopen(arg)
    ls = readfile("/tmp/active_dirs.txt")
    if ls:
        for l in reversed(ls.split("\n")):
            add(["recentactivedirs", l], _, cmds)

    def _(arg, path, rofi):
        run_shell_async("sleep 0.1 && xdotool type ' | xclip -selection clipboard' ")
    add(["clipcmd",], _, cmds)

    def _(arg, path, rofi):
        browseropen("https://www.crxsoso.com/")
    add(["crx",], _, cmds)

    def _(arg, path, rofi):
        arg = arg.replace("|||", "\n")
        copy(arg)
        if arg.strip().startswith("http"):
            browseropen(arg)
        if arg.strip().startswith("/"):
            run_shell_async(f"st -d {arg}")

    # home = os.environ["HOME"]
    # contextdir = os.path.join(home, ".config/collector")
    contextdir = "/tmp"
    ls = readfile(f"{contextdir}/recent_context.txt")
    if ls:
        for l in reversed(ls.split("\n")):
            add(["context", l], _, cmds)

    def _(arg, path, rofi):
        copy("pip3 install -i https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple ")
        run_shell_async("sleep 1 && xdotool key 'shift+Insert'")
    add(["pip",], _, cmds)

    def _(arg, path, rofi):
        # r = run_shell("cat /dev/urandom | sed 's/[^a-zA-Z]//g' | strings -n 6 | head -n 1")
        r = "".join(random.sample("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXY", 6) )
        copy_x(r.strip())
    add(["randomstring",], _, cmds)

    def _(arg, path, rofi):
        r = run_shell_async("dwm-msg run_command nextmanagetype 2 && browserclip.sh \"http://youdao.com/result?word=%s&lang=en\"")
    add(["fanyi",], _, cmds, topidentifyfunc=regex_topidentifyfunc(r"^[a-zA-Z]{3,18}$"))

    def _(arg, path, rofi):
        r = run_shell_async("dwm-msg run_command nextmanagetype 2 && browserclip.sh \"https://g.savalone.com/search?q=%s\"")
    add(["googlesearchtmp",], _, cmds)

    def _(arg, path, rofi):
        clip = getprimary()
        clip = quote(clip)
        browseropen(f"https://g.savalone.com/search?q={clip}")
    add(["googlesearch",], _, cmds, forcetop=9998)

    def _(arg, path, rofi):
        browseropen("https://gitee.com/beyondlov1/mytools/tree/master")
    add(["mytools_gitee",], _, cmds)

    # def _(arg, path, rofi):
    #     browseropen("https://tool.lu/timestamp/")
    # add(["shijianchuo",], _, cmds, topidentifyfunc=regex_topidentifyfunc(r"\d{10}|^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$"))

    def _(arg, path, rofi):
        clip = getclipboard()
        if re.match(r"\d{10}", clip):
            timestr = timestamp2datestr(int(clip))
            simplepageshow(timestr)
        if re.match(r"^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$", clip):
            timestamp = datestr2timestamp(clip)
            copy(timestamp)
            simplepageshow(timestamp)
    add(["shijianchuo2",], _, cmds, topidentifyfunc=regex_topidentifyfunc(r"\d{10}|^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$"))

    def _(arg, path, rofi):
        browseropen("https://www.json.cn/")
    add(["json",], _, cmds)

    def _(arg, path, rofi):
        clip = getclipboard()
        copy(clip.replace(",", ""))
    add(["removecomma",], _, cmds, topidentifyfunc=regex_topidentifyfunc(r"\d+,\d+\.?\d*"))

    def _(arg, path, rofi):
        githubdir = "/home/beyond/github/"
        if not os.path.exists(githubdir):
            githubdir = "/home/beyond/Documents/GitHubProject"
        stopen(githubdir)
        clip = getclipboard()
        copy(f"git clone {clip} --depth 1")
        run_shell_async("sleep 1 && xdotool key 'shift+Insert'")
    add(["gitclone",], _, cmds, topidentifyfunc=regex_topidentifyfunc(r"(http|git).*git"))

    def _(arg, path, rofi):
        clip = getclipboard()
        name = clip.split("/")[-1]
        cmd = f"wgetmulti.sh {clip} {name}"
        copy(cmd)
    add(["multiget",], _, cmds, topidentifyfunc=regex_topidentifyfunc(r"(?!.*\.html$)http.*\..*$"))

    # 貌似没用
    # def _(arg, path, rofi):
    #     run_shell_async("""sudo sh -c 'sed -i "/# GitHub520 Host Start/Q" /etc/hosts && curl https://raw.hellogithub.com/hosts >> /etc/hosts'""")
    # add(["updategithubhosts",], _, cmds, topidentifyfunc=regex_topidentifyfunc(r"(?!.*\.html$)http.*\..*$"))

    def _(arg, path, rofi):
        clip = getclipboard()
        clip = clip.replace("\n", " ")
        clip = clip.replace("\t", " ")
        clip = clip.strip()
        l = clip.split(" ")
        liststr = ",".join([f"\"{item}\"" for item in set(l)])
        copy(liststr)
    add(["line2list",], _, cmds)


    def _(arg, path, rofi):
        copy("""pd.set_option('display.max_columns', None)
pd.set_option('display.width', None)
pd.set_option('display.max_colwidth',None)
# pd.set_option('display.max_rows', None)""")
    add(["pdoptions",], _, cmds)
    
