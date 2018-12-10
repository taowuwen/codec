#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import absolute_import

import parser.quanben as quanben
from parser.httpdownload import _http
from parser.httpdownload import _https
from urllib.parse import urljoin
from parser.httpdownload import url_download
from parser.httpdownload import HTTPDownload
from parser.httpdownload import HTTPDownload_V1
import re
import os
from bs4 import BeautifulSoup
import bs4
from collections import OrderedDict

_url_path = 'www.booktxt.net'
_url_root = _https + _url_path

class Booktxt(HTTPDownload_V1):
    pass

class PageDownload(Booktxt):

    def parse_get(self, ctx):

        self._info = {}

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            _bs = _bs.find(class_='box_con')

            self._get_title(_bs)
            self._get_content(_bs)
            self._get_urls(_bs)

        except Exception as e:
            print("Exception: {}".format(e))
            raise quanben.InvalidPageInfo("Invalid page info")

        return self._info


    def _get_title(self, bs):

        try:
            bs = bs.find(class_='bookname')
            self._info["title"] = bs.h1.string.strip()
        except ValueError:
            raise quanben.TitleNotFound("title not found")


    def _get_content(self, bs):

        try:
            obj = bs.find(id='content')

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
            obj = bs.find(class_='bottem2')

            if obj:
                urls = obj.find_all('a')

                self._info["page_prev"] = urls[1].get('href')
                self._info["menu"]      = urls[2].get('href')
                self._info["page_next"] = urls[3].get('href')

        except ValueError:
            pass
    
class MenuDownload(Booktxt):

    def parse_get(self, ctx):

        self._items = OrderedDict()
        #self._items = {}

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            menu_node = _bs.find(id='list')

            first_entry = 2

            for m in menu_node.dl.children:

                if first_entry:
                    if type(m) is bs4.element.Tag:
                        if m.name == 'dt':
                            first_entry -= 1

                    continue

                if type(m) is bs4.element.NavigableString:
                    continue
                else:
                    self._items[str(m.string).strip()] = m.a.get('href')

        except Exception as e:
            pass

        if self._items:
            return self._items

        raise quanben.InvalidPageInfo("Invalid Menu Page")


class URLDownload(url_download):
    pass

def _main():

   # page = PageDownload()
   # print(_url_root)
   # url = 'https://www.booktxt.net/1_1562/508673.html'
   # info = page.http_get(url)
   # print(info)


    menu = MenuDownload()
    print(_url_root)
    url = 'https://www.booktxt.net/1_1562/'
    info = menu.http_get(url)
    print(info)


if __name__ == '__main__':
    _main()
