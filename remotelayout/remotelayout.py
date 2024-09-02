from http.server import HTTPServer, BaseHTTPRequestHandler, SimpleHTTPRequestHandler
import os
import sys
import uuid
import time
import collections
import copy
import json


data = {'result': 'this is a test'}
host = ('0.0.0.0', 7397)


import subprocess
import re
# from Xlib import display,X

lastwinlist = []

def listwindow():
    listwin_cmd = "wmctrl -l -G -x"
    winlist_return = subprocess.run(listwin_cmd, stdout = subprocess.PIPE, stderr = subprocess.PIPE, encoding ='utf-8' ,shell =True)

    getfocus_cmd = "xdotool getwindowfocus"
    getfocus_return = subprocess.run(getfocus_cmd, stdout = subprocess.PIPE, stderr = subprocess.PIPE, encoding ='utf-8' ,shell =True)
    focus_wid_int = int(getfocus_return.stdout)
    
    winliststr = winlist_return.stdout

    winlist = []
    winlistlines = winliststr.split("\n")
    for line in winlistlines:
        if not line.strip():
            continue
        line = re.sub(r" +"," ", line)
        items = line.split(" ")
        wid = items[0]
        x = int(items[2])/2 # 不知道为什么wmctrl 命令的到的xy会是2倍， xdotool 得到的是对的
        y = int(items[3])/2
        w = int(items[4])
        h = int(items[5])
        cls = items[6]
        namelist = []
        for i in range(8,len(items)):
            namelist.append(items[i])
        name = " ".join(namelist)
        focused = 0
        if int(wid,16) == focus_wid_int:
            focused = 1
        winlist.append({"x":x,"y":y,"w":w,"h":h,"name":name,"wid":wid,"focused":focused,"class":cls})

    return winlist

def listwindow2():
    listwin_cmd = "dwm-msg get_dwm_clients"
    listwin_return = subprocess.run(listwin_cmd, stdout = subprocess.PIPE, stderr = subprocess.PIPE, encoding ='utf-8' ,shell =True)
    winlist = json.loads(listwin_return.stdout)
    result = []
    for item in winlist:
        g = item["geometry"]["current"]
        x = g["x"]
        y = g["y"]
        w = g["width"]
        h = g["height"]
        name = item["name"]
        wid = hex(item["window_id"])
        focused = item["states"]["is_focused"]
        cls = item["class"]
        if item["states"]["is_floating"]:
            continue
        result.append({"x":x,"y":y,"w":w,"h":h,"name":name,"wid":wid,"focused":focused,"class":cls})
    return result


class Resquest(SimpleHTTPRequestHandler):
    
    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'X-Requested-With, Content-Type')
        self.end_headers()

        # Respond with an empty body to the pre-flight request
        self.wfile.write(b'')

    def resp(self, data):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'X-Requested-With, Content-Type')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())
    
    def do_POST(self):
        print('headers', self.headers)
        print("do post:", self.path, self.client_address, )

        datas = self.rfile.read(int(self.headers['content-length']))
        datasobj = json.loads(str(datas, encoding='utf-8'))
        global lastwinlist

        if self.path == "/focus":
            wid = datasobj["wid"]
            focus_cmd = "xdotool windowactivate " + wid
            subprocess.run(focus_cmd, shell=True)
            time.sleep(0.2)
            for win in lastwinlist:
                if win["wid"] == wid:
                    win["focused"] = 1
                else:
                    win["focused"] = 0
            winlist = lastwinlist
            self.resp({"size":len(winlist), "content":winlist})

        if self.path == "/list":
            winlist = listwindow2()
            lastwinlist = copy.deepcopy(winlist)
            self.resp({"size":len(winlist), "content":winlist})

        if self.path == "/cachedlist":
            self.resp({"size":len(lastwinlist), "content":lastwinlist})

        if self.path == "/hotkey":
            hotkey = datasobj["hotkey"]
            hotkey_cmd = "xdotool key " + hotkey
            subprocess.run(hotkey_cmd, shell=True)
            self.resp({})
        if self.path == "/command":
            subprocess.run( datasobj["command"], shell=True)
            self.resp({})

if __name__ == '__main__':  
    from functools import partial
    htmlhome = os.path.join(os.getcwd(), os.path.dirname(sys.argv[0]))
    RequestHandlerClass = partial(Resquest,directory=htmlhome)
    server = HTTPServer(host, RequestHandlerClass)
    print("Starting server, listen at: %s:%s" % host)
    server.serve_forever()
