#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import absolute_import

import parser.quanben as quanben
import parser.biquge as biquge
from parser.httpdownload import _http
from parser.httpdownload import _https
from urllib.parse import urljoin
from parser.httpdownload import url_download
from parser.httpdownload import HTTPDownload
import re
import os
from bs4 import BeautifulSoup
import bs4
from collections import OrderedDict

_url_path = "www.qiushuge.net"
_url_root = _http + _url_path


class QiuShuGe(biquge.BiQuGe):
    pass


class PageDownload(QiuShuGe):
    def parse_get(self, ctx):

        self._info = {}

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            _bs = _bs.find("div", class_='bg')

            self._get_title(_bs)
            self._get_content(_bs)
            self._get_urls(_bs)

        except Exception as e:
            raise quanben.InvalidPageInfo("Invalid page info")

        return self._info

    def _get_title(self, bs):

        try:
            self._info["title"] = bs.h1.string.strip()
        except ValueError:
            raise quanben.TitleNotFound("Title not found")

    def _get_content(self, bs):

        try:
            bs = bs.find("div", class_="content")

            self._info["content"] = ""

            p = bs.p
            while p:

                if type(p) is bs4.element.NavigableString:
                    p = p.next_sibling
                    continue

                if type(p) is bs4.element.Tag:
                    if p.name != "p":
                        break
                    else:
                        if p.string:
                            self._info["content"] += "\n\t" + str(p.string).strip() + "\n"
                        else:
                            for l in p.children:
                                if l.string:
                                    self._info["content"] += "\n\t" + str(l.string).strip() + "\n"

                p = p.next_sibling

        except ValueError:
            raise quanben.InvalidPageInfo("Content Error")

    def _get_urls(self, bs):
        try:
            menu = bs.find("div", class_='mark')

            self._info["menu"] = menu.a.get("href")
            self._info["page_prev"] = self._info["menu"]
            self._info["page_next"] = self._info["menu"]

        except ValueError:
            raise quanben.InvalidPageInfo("Did not found menu url")

        try:
            url = bs.find("div", class_="content")
            urls = url.find_all('a')

            for _u in urls:

                pos = _u.get("rel", None)
                if not pos:
                    continue

                if pos[0] == "next":
                    self._info["page_next"] = _u["href"]
                elif pos[0] == "prev":
                    self._info["page_prev"] = _u["href"]

        except ValueError:
            raise quanben.InvalidPageInfo("Page Error")


class MenuDownload(QiuShuGe):
    def parse_get(self, ctx):

        self._items = OrderedDict()

        try:
            _bs = BeautifulSoup(ctx, "html.parser")

            menu_node = _bs.find("div", class_="panel")

            for m in menu_node.ul.children:

                if type(m) is bs4.element.NavigableString:
                    continue
                else:
                    self._items[str(m.string).strip()] = m.a.get('href')

        except Exception as e:
            pass

        if self._items:
            return self._items

        raise quanben.InvalidPageInfo("Invalid Menu Page")


class BookInfoDownload(QiuShuGe):
    pass


class URLDownload(url_download):
    pass


def _main():

    url = 'http://www.qiushuge.net/zhongjidouluo/20.html'
    url = 'http://www.qiushuge.net/zhongjidouluo/4.html'
    #url = 'http://www.qiushuge.net/zhongjidouluo/588.html'
    url = 'http://www.qiushuge.net/zhongjidouluo/62.html'
    url = 'http://www.qiushuge.net/zhongjidouluo/584.html'
    page = PageDownload()
    info = page.http_get(url)
    print(info)

# url = 'http://www.qiushuge.net/zhongjidouluo/'
# menu = MenuDownload()
# info =  menu.http_get(url)

# print(info)
# for i,v in enumerate(info):
#     print("{}, {}, {}".format(i+1, v, info[v]))

if __name__ == '__main__':
    _main()
