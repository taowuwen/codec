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

_url_path = 'www.luoxia.com'
_url_root = _http + _url_path


class LuoXia(HTTPDownload):
    def http_get(self, url):
        return super().http_get(urljoin(_url_root, url))

    def http_post(self, url, data):
        return super().http_post(urljoin(_url_root, url), data)


class PageDownload(LuoXia):
    def parse_get(self, ctx):

        self._info = {}

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            self._get_title(_bs.body)
            self._get_content(_bs.body)
            self._get_urls(_bs.body)

        except Exception as e:
            print("Exception: {}".format(e))
            raise InvalidPageInfo("Invalid page info")

        return self._info

    def _get_title(self, bs):

        try:
            bs = bs.find(class_='post-title')
            self._info["title"] = bs.string.strip()
        except ValueError:
            raise quanben.TitleNotFound("title not found")

    def _get_content(self, bs):

        try:
            obj = bs.find("div", id='nr1')

            if not obj:
                raise quanben.PageContentNotFound("page content not found")

            self._info["content"] = ""

            for l in obj.children:
                if l.string and str(l.string).strip():
                    self._info["content"] += "\n\t" + str(
                        l.string).strip() + "\n"

        except ValueError:
            raise quanben.PageContentNotFound("page content not found")

    def _get_urls(self, bs):

        try:

            obj = bs.find(id="header2")

            if obj:
                urls = obj.find_all('a')
                self._info["menu"] = urls[2]['href']
            else:
                raise quanben.InvalidPageInfo("Did not find menu info")

            bs = bs.find(class_="nav2 bbn mb2 clearfix")

            # prev
            obj = bs.find(class_="prev")

            if obj and obj.a:
                self._info["page_prev"] = obj.a['href'].strip()
            else:
                self._info["page_prev"] = self._info["menu"]

            # next
            obj = bs.find(class_="next")

            if obj and obj.a:
                self._info["page_next"] = obj.a['href'].strip()
            else:
                self._info["page_next"] = self._info["menu"]

        except ValueError:
            pass


class MenuDownload(LuoXia):
    def parse_get(self, ctx):

        self._items = OrderedDict()

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            menu_node = _bs.find("div", class_="book-list clearfix")

            for m in menu_node.ul.children:
                if type(m) is bs4.element.NavigableString:
                    continue
                else:
                    _key = None
                    _val = None
                    if m.a:
                        _key = str(m.a.string).strip()
                        _val = m.a.get('href')
                    else:
                        _key = str(m.b.string).strip()
                        _val = m.b.get('onclick')

                        _start = "window.open('"
                        _end = "')"

                        _val = _val[len(_start):-len(_end)]

                    if _key and _val:
                        self._items[_key] = _val

        except Exception as e:
            print(e)
            pass

        if self._items:
            return self._items

        raise InvalidPageInfo("Invalid Menu Page")


class URLDownload(url_download):
    pass


def _main():

     page = PageDownload()
     print(_url_root)
     url = 'http://www.luoxia.com/tangmen/4494.htm'
     #url = 'http://www.luoxia.com/tangmen/5113.htm'
     #url = 'http://www.luoxia.com/tangmen/14769.htm'
     info = page.http_get(url)
     print(info)

    #   menu = MenuDownload()
    #   print(_url_root)
    #   url = 'http://www.luoxia.com/tangmen/'
    #   info = menu.http_get(url)
    #   print(info)


if __name__ == '__main__':
    _main()
