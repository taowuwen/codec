#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import absolute_import

import parser.quanben as quanben
from parser.httpdownload import _http
from parser.httpdownload import _https
from urllib.parse import urljoin
from parser.httpdownload import url_download
from parser.httpdownload import HTTPDownload
import re
import os
from bs4 import BeautifulSoup
from parser.quanben import InvalidPageInfo

_url_path = 'www.23us.so'
_url_root = _https + _url_path

class C23US(HTTPDownload):

    def http_get(self, url):
        return super().http_get(urljoin(_url_root, url))

    def http_post(self, url, data):
        return super().http_post(urljoin(_url_root, url), data)


class PageDownload(C23US):

    def parse_get(self, ctx):

        self._info = {}

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            _bs = _bs.find(id='amain')

            self._get_title(_bs)

            self._get_content(_bs)
            self._get_urls(_bs)

        except Exception as e:
            print("Exception: {}".format(e))
            raise InvalidPageInfo("Invalid page info")

        return self._info


    def _get_title(self, bs):

        try:
            self._info["title"] = bs.dl.dd.h1.string.strip()
        except ValueError:
            raise quanben.TitleNotFound("title not found")


    def _get_content(self, bs):

        try:
            obj = bs.find(id='contents')

            if not obj:
                raise quanben.PageContentNotFound("page content not found")

            self._info["content"] = ""

            for l in obj.children:
                if l.string and str(l.string).strip():
                    self._info["content"] += "\n\t" + str(l.string).strip() + "\n"


        except ValueError:
            raise quanben.PageContentNotFound("page content not found")

    def _get_urls(self, bs):

        try:
            obj = bs.find(id='footlink')

            if obj:
                urls = obj.find_all('a')

                self._info["page_prev"] = urls[0].get('href')
                self._info["menu"]      = urls[1].get('href')
                self._info["page_next"] = urls[2].get('href')

        except ValueError:
            pass
    
class MenuDownload(C23US, quanben.MenuDownload):
    from collections import OrderedDict

    def parse_get(self, ctx):

        self._items = {}

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            menu_list = _bs.find_all(class_='L')

            for m in menu_list:
                self._items[str(m.string).strip()] = m.a.get('href')

        except Exception as e:
            print("Exception: {}".format(e))

        if self._items:
            return self._items

        raise InvalidPageInfo("Invalid Menu Page")


class URLDownload(url_download):
    pass

def _main():

   # page = PageDownload()
   # print(page._url_root)
   # url = 'https://www.23us.so/files/article/html/30/30177/13921970.html'
   # info = page.http_get(url)
   # print(info)

    menu = MenuDownload()
    print(menu._url_root)
    url = 'https://www.23us.so/files/article/html/30/30177/index.html'
    info = menu.http_get(url)
    print(info)


if __name__ == '__main__':
    _main()
