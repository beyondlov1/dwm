from http.server import HTTPServer, BaseHTTPRequestHandler
import smartwin
from urllib.parse import urlparse, parse_qs
import json

data = {'result': 'this is a test'}
host = ('localhost', 8666)



 
class Resquest(BaseHTTPRequestHandler):
    timeout = 5
    server_version = "Apache"   #设置服务器返回的的响应头 

    def resort(self):
        self.send_response(200)
        self.send_header("Content-type","text/plain")
        self.end_headers()
        # clients = self.headers.get_param("clients")
        # query = parse_qs(urlparse(self.path).query)
        query = parse_qs(self.rfile.read(int(self.headers['content-length'])).decode("utf-8"))
        print(query)
        # 获取参数值
        # names = query.get("names")
        # names = names[0].split(",")
        # classes = query.get("classes")
        # classes = classes[0].split(",")
        # print(names,classes)
        # resorted = smartwin.resort(model, (names, classes))

        launchparents = query.get("launchparents")
        launchparents = list(map(int, launchparents[0].split(",") ))
        tag = query.get("tag")
        tag = int(tag[0])
        print(launchparents)
        resorted = smartwin.resort2(tag,launchparents)
        buf = ",".join(list(map(str,resorted)))

        self.wfile.write(buf.encode())  #里面需要传入二进制数据，用encode()函数转换为二进制数据   #设置响应body，即前端页面要展示的数据

    def place(self):
        self.send_response(200)
        self.send_header("Content-type","text/plain")
        self.end_headers()
        query = parse_qs(self.rfile.read(int(self.headers['content-length'])).decode("utf-8"))
        print(query)
        tag = query.get("tag")
        tag = int(tag[0])
        targets = query.get("targets")
        targets = targets[0].split(",")
        targetpairs = []
        for target in targets:
            arr = target.split("|")
            arr = list(map(int, arr))
            targetpairs.append((arr[0], arr[1], arr[2]))
        print(targets)
        smartwin.place(tag,targetpairs)
        buf = ""

        self.wfile.write(buf.encode())  #里面需要传入二进制数据，用encode()函数转换为二进制数据   #设置响应body，即前端页面要展示的数据

    def do_POST(self):
        print(self.path)
        if(self.path == "/resort"):
            self.resort()
        if(self.path == "/place"):
            self.place()

 
 
if __name__ == '__main__':
    # model = smartwin.train()
    server = HTTPServer(host, Resquest)
    print("Starting server, listen at: %s:%s" % host)
    server.serve_forever()
