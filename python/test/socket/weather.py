#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import sys
import binascii


REQUEST = '''GET /v3/weather/weatherInfo?city={city}&key={key}&extensions=all HTTP/1.1\r
Host: {host}\r
User-Agent: Mozilla/5.0 Firefox/45.0\r
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r
Accept-Encoding: gizp, deflate\r
Cache-Control: no-cache\r
\r
'''

# http://restapi.amap.com/v3/weather/weatherInfo?city=110101&key=<用户key>

KEY = u'2383d460b0c9def51819247bdaa18c41'
HOST = u"restapi.amap.com"
CITY = u"110101"


s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    s.connect((HOST, 80))
    request = REQUEST.format(host=HOST, city=CITY, key=KEY)
    print(request)

    s.sendall(request.encode('utf-8'))

    while True:
        chunk = s.recv(1024)

        if chunk:
            print('{}'.format(chunk.decode('utf-8')))
        else:
            break

except socket.error as msg:
    print(msg)
finally:
    s.close()
