import time
import requests
import pyperclip
import binascii
import sys
import os
import json


# host = "10.52.1.80"
host = "localhost"
port = 8667
puturl = f"http://{host}:{port}/put"
geturl = f"http://{host}:{port}/get"
lastclipdata = ""
lastcliptime = 0
locallastclipdata = ""
while True:
    clipdata = pyperclip.paste()
    if clipdata and clipdata != locallastclipdata:
        print(clipdata)
        locallastclipdata = clipdata
        lastclipdata = clipdata
        lastcliptime = time.time()
        requests.post(url=puturl, headers={"Content-Type":"application/json"}, data=json.dumps({"content":clipdata, "time":lastcliptime}))
    remotedatadict = requests.post(url=geturl)
    remotedatadict = remotedatadict.json()
    remotedata = remotedatadict["content"]
    remotetime = remotedatadict["time"]
    if remotetime > lastcliptime:
        lastclipdata = remotedata
        lastcliptime = remotetime
        pyperclip.copy(remotedata)
    time.sleep(0.2)
    
    