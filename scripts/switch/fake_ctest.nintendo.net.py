# simple python3 http server to emulate ctest.nintendo.net
# you have to redirect nintendo.net to your own host
# https://gitlab.com/a/90dns

# the aim is to fake ctest.nintendo.net server
# to allow nintendo switch lan connection

# The nintendo switch tries to join ctest to validate wifi settings
# without 200 OK from ctest.nintendo.net, the LAN connection is denied

import http.server
import socketserver

from http.server import HTTPServer, BaseHTTPRequestHandler


class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("X-Organization", "Nintendo")
        self.end_headers()
        self.wfile.write(b'ok')


PORT = 80

httpd = HTTPServer(('0.0.0.0', PORT), SimpleHTTPRequestHandler)
httpd.serve_forever()

