from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import json
import time


host = ('0.0.0.0', 8667)

data = ""
lasttime = 0

class Resquest(BaseHTTPRequestHandler):
    timeout = 5
    server_version = "Apache"   #设置服务器返回的的响应头 

    def get(self):
        global lasttime, data
        #req_datas = self.rfile.read(int(self.headers['content-length'])) 
        #res1 = req_datas.decode('utf-8')
        #res = json.loads(res1)

        respdata = {"content":data,"time":lasttime}
        respdatastr = json.dumps(respdata)

        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(respdatastr.encode('utf-8'))

    def put(self):
        global lasttime, data
        req_datas = self.rfile.read(int(self.headers['content-length'])) 
        res1 = req_datas.decode('utf-8')
        # print(res1)
        res = json.loads(res1)

        if lasttime > res["time"]:
            respdata = {"status":0}
        else:
            data = res["content"]
            lasttime = time.time()
            respdata = {"status":1, "content":data, "time":lasttime}

        
        respdatastr = json.dumps(respdata)

        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(respdatastr.encode('utf-8'))


    def do_POST(self):
        # print(self.path)
        if(self.path == "/put"):
            self.put()
        if(self.path == "/get"):
            self.get()

 
 
if __name__ == '__main__':
    # model = smartwin.train()
    server = HTTPServer(host, Resquest)
    print("Starting server, listen at: %s:%s" % host)
    server.serve_forever()
