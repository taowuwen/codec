#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import json
from http_async import HttpDownloader
from http_async import InvalidHttpResponse

import argparse


CITY_CODE=510100    # chengdu
KEY_SECRECT="2383d460b0c9def51819247bdaa18c41"
URL='http://restapi.amap.com/v3/weather/weatherInfo?city={city}&key={key}'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="this is my argparse test")

    parser.add_argument('-k', '--key', action='store', dest='key', default=KEY_SECRECT)
    parser.add_argument('-c', '--citycode', action='store', dest='city', default=CITY_CODE)

    argument = parser.parse_args()

    http = HttpDownloader(URL.format(city=argument.city, key=argument.key))

    http.run()

    if http.body:
        body = http.body.decode()
        jobj = json.loads(body)

        print(jobj)



