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
import bs4
from parser.quanben import InvalidPageInfo
from collections import OrderedDict

_url_path = 'www.shuquge.com'
_url_root = _http + _url_path

class ShuQuge(HTTPDownload):

    def http_get(self, url):
        return super().http_get(urljoin(_url_root, url))

    def http_post(self, url, data):
        return super().http_post(urljoin(_url_root, url), data)


class PageDownload(ShuQuge):

    def parse_get(self, ctx):

        self._info = {}

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            _bs = _bs.find(class_='content')

            self._get_title(_bs)
            self._get_content(_bs)
            self._get_urls(_bs)

        except Exception as e:
            print("Exception: {}".format(e))
            raise InvalidPageInfo("Invalid page info")

        return self._info


    def _get_title(self, bs):

        try:
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
            obj = bs.find(class_='page_chapter')

            if obj:
                urls = obj.find_all('a')

                self._info["page_prev"] = urls[0].get('href')
                self._info["menu"]      = urls[1].get('href')
                self._info["page_next"] = urls[2].get('href')

        except ValueError:
            pass
    
class MenuDownload(ShuQuge):

    def parse_get(self, ctx):

        self._items = OrderedDict()
        skip_2_dt = 2

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            menu_node = _bs.find(class_='listmain')

            for m in menu_node.dl.children:
                if type(m) is bs4.element.NavigableString:
                    continue
                else:
                    if hasattr(m.a, 'get'):
                        if skip_2_dt <= 0:
                            self._items[str(m.string).strip()] = m.a.get('href')
                    else:
                        skip_2_dt -= 1


        except Exception as e:
            pass

        if self._items:
            return self._items

        raise InvalidPageInfo("Invalid Menu Page")


class URLDownload(url_download):
    pass

def _main():

    url = 'http://www.shuquge.com/txt/85646/18629526.html'
    url = 'http://www.shuquge.com/txt/85646/18629525.html'
    url = 'http://www.shuquge.com/txt/85646/29300289.html'

    page = PageDownload()
    print(_url_root)
    info = page.http_get(url)
    print(info)

  # menu = MenuDownload()
  # print(_url_root)
  # url = 'http://www.shuquge.com/txt/85646/index.html'
  # info = menu.http_get(url)
  # print(info)


if __name__ == '__main__':
    _main()
