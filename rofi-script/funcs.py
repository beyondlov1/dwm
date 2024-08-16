__all__ = [
    "addclipboard",
    "addoptions",
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

def addoptions(cmds):
    def _showclipboards(arg,path,cmds):
        run_shell_async("sleep 0.1 && /bin/clipcat-menu")
    add(["clipboard",], _showclipboards, cmds)
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
    add(["togithubmirror",], _, cmds)
    def _(arg, path, rofi):
        r = _fetchssr()
        copy(r)
    add(["fetchssr",], _, cmds)