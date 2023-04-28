from http.server import HTTPServer, BaseHTTPRequestHandler
import smartwin
from urllib.parse import urlparse, parse_qs
import json

data = {'result': 'this is a test'}
host = ('localhost', 8666)
 
class Resquest(BaseHTTPRequestHandler):
    timeout = 5
    server_version = "Apache"   #设置服务器返回的的响应头 
    def do_POST(self):
        self.send_response(200)
        self.send_header("Content-type","text/plain")
        self.end_headers()
        buf = "hello world"
        # clients = self.headers.get_param("clients")
        # query = parse_qs(urlparse(self.path).query)
        query = parse_qs(self.rfile.read(int(self.headers['content-length'])).decode("utf-8"))
        print(query)
        # 获取参数值
        names = query.get("names")
        names = names[0].split(",")
        classes = query.get("classes")
        classes = classes[0].split(",")
        # clients = self.headers.get_params()
        # print(clients)
        print(names,classes)
        resorted = smartwin.resort(model, (names, classes))
        buf = ",".join(list(map(str,resorted)))

        self.wfile.write(buf.encode())  #里面需要传入二进制数据，用encode()函数转换为二进制数据   #设置响应body，即前端页面要展示的数据
 
 
if __name__ == '__main__':
    model = smartwin.train()
    server = HTTPServer(host, Resquest)
    print("Starting server, listen at: %s:%s" % host)
    server.serve_forever()
