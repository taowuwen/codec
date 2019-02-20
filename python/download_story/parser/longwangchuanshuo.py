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

_url_path = 'www.longwangchuanshuo.com'
_url_root = _http + _url_path


class DouLuoDaLu(HTTPDownload):
    def http_get(self, url):
        return super().http_get(urljoin(_url_root, url))

    def http_post(self, url, data):
        return super().http_post(urljoin(_url_root, url), data)


class PageDownload(DouLuoDaLu):
    def parse_get(self, ctx):

        self._info = {}

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            self._get_title(_bs.body)
            self._get_content(_bs.body)
            self._get_urls(_bs)

        except Exception as e:
            print("Exception: {}".format(e))
            raise InvalidPageInfo("Invalid page info")

        return self._info

    def _get_title(self, bs):

        try:
            bs = bs.find(class_='post_title')
            self._info["title"] = bs.h1.string.strip()
        except ValueError:
            raise quanben.TitleNotFound("title not found")

    def _get_content(self, bs):

        try:
            obj = bs.find(class_='post_entry')

            if not obj:
                raise quanben.PageContentNotFound("page content not found")


            _ctx = "{}".format(obj)
            _tag = '<div class="post_entry">'
            _tag_close = '</div>'

            _ctx = _ctx[len(_tag):-len(_tag_close)]
            lines = _ctx.replace("</p>", "").split("<p>")

            self._info["content"] = ""

            for l in lines:
                if l and l.strip():
                    self._info["content"] += "\n\t" + l.strip() + "\n"

          # for l in obj.children:
          #     if l.string and str(l.string).strip():
          #         self._info["content"] += "\n\t" + str(
          #             l.string).strip() + "\n"

        except ValueError:
            raise quanben.PageContentNotFound("page content not found")

    def _get_urls(self, bs):

        try:

            obj = bs.find(class_='post_title')

            if obj:
                self._info["menu"] = obj.span.a['href'].strip()
            else:
                raise quanben.InvalidPageInfo("Did not find menu info")

            bs = bs.find(class_="post-nav")

            # prev
            obj = bs.find(class_="fanyes")

            if obj and obj.a:
                self._info["page_prev"] = obj.a['href'].strip()
            else:
                self._info["page_prev"] = self._info["menu"]

            # next
            obj = bs.find(class_="fanyex")

            if obj and obj.a:
                self._info["page_next"] = obj.a['href'].strip()
            else:
                self._info["page_next"] = self._info["menu"]

        except ValueError:
            pass


class MenuDownload(DouLuoDaLu):
    def parse_get(self, ctx):

        self._items = OrderedDict()

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            menu_node = _bs.find_all("div", class_="container")[1]

            for m in menu_node.ul.children:

                if type(m) is bs4.element.NavigableString:
                    continue
                else:
                    self._items[str(m.string).strip()] = m.a.get('href')

        except Exception as e:
            pass

        if self._items:
            return self._items

        raise InvalidPageInfo("Invalid Menu Page")


class URLDownload(url_download):
    pass


def _main():

    page = PageDownload()
    print(_url_root)
    #url = 'http://www.longwangchuanshuo.com/zhongji/136.html'
    url = 'http://www.longwangchuanshuo.com/douluodalu/1.html'
    #url = 'http://www.longwangchuanshuo.com/douluodalu/2.html'
    #url = 'http://www.longwangchuanshuo.com/douluodalu/610.html'
    info = page.http_get(url)
    print(info)


# menu = MenuDownload()
# print(_url_root)
# url = 'http://www.longwangchuanshuo.com/douluodalu/'
# info = menu.http_get(url)
# print(info)

if __name__ == '__main__':
    _main()
