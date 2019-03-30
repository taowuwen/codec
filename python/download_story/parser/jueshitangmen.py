#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from parser.httpdownload import HTTPDownload_V1
from parser.httpdownload import url_download
from parser.httpdownload import _http
from parser.httpdownload import _https
from parser.qiushuge import PageDownload as qiushuge_PageDownload
from parser.qiushuge import MenuDownload as qiushuge_MenuDownload
from parser.qiushuge import BookInfoDownload as qiushuge_BookInfo
import parser.quanben as quanben

from urllib.parse import urljoin
import sys
import re
from bs4 import BeautifulSoup
import bs4


_url_path = "www.jueshitangmen.info"
_url_root = _http + _url_path


class JueShiTangMen(HTTPDownload_V1):
    pass


class PageDownload(JueShiTangMen, qiushuge_PageDownload):

    def parse_get(self, ctx):

        self._info = {}

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            bs = _bs.find("div", class_='bg')

            self._get_title(bs)
            self._get_content(bs)
            self._get_urls(_bs)

        except Exception as e:
            raise quanben.InvalidPageInfo("Invalid page info")

        return self._info

    def _get_urls(self, bs):
        
        try:
            menu = bs.find("div", class_="banner")

            self._info["menu"] = menu.a.get("href")
            self._info["page_prev"] = self._info["menu"]
            self._info["page_next"] = self._info["menu"]

        except ValueError:
            raise quanben.InvalidPageInfo("invalid page info")


        try:
            url = bs.find("div", class_="content")
            urls = url.center.find_all('a')

            for _u in urls:

                pos = _u.get("rel", None)
                if not pos:
                    continue

                if pos[0] == "next":
                    self._info["page_next"] = _u["href"]
                elif pos[0] == "prev":
                    self._info["page_prev"] = _u["href"]

        except ValueError:
            raise quanben.InvalidPageInfo("invalid page info")



class MenuDownload(JueShiTangMen, qiushuge_MenuDownload):
    pass


class BookInfoDownload(JueShiTangMen, qiushuge_BookInfo):
    pass


class URLDownload(url_download):
    pass


def _main():

 #  url = 'http://www.jueshitangmen.info/wudongqiankun/3.html'
 #  url = 'http://www.jueshitangmen.info/yinianyongheng/583.html'
 #  page = PageDownload()
 #  print(page.http_get(url))


    url = 'http://www.jueshitangmen.info/yinianyongheng/'
#   page.http_post("http://localhost:8000/cgi-bin/hello.py", {"foo":"bar"})
    menu = MenuDownload()
    info = menu.http_get(url)

    for index, k in enumerate(info):
        print("{:<}\t: {:<30s}{:<}".format(index, k, info[k]))
#    print(menu.http_get('http://www.jueshitangmen.info/wudongqiankun/'))
#   book = BookInfoDownload()
#   book.http_get('n/jiuzhuanhunchunjue/')

if __name__ == '__main__':
    _main()
