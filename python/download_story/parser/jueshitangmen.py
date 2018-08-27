#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from parser.httpdownload import HTTPDownload
from parser.httpdownload import url_download
from parser.httpdownload import _http
from parser.httpdownload import _https

from urllib.parse import urljoin
import sys
import re


class TitleNotFound(Exception):
    pass


class PageContentNotFound(Exception):
    pass


class PageScriptNotFound(Exception):
    pass


class InvalidPageInfo(Exception):
    pass

class MethodNotImpletion(Exception):
    pass


class JueShiTangMen(HTTPDownload):
    _url_path = "www.jueshitangmen.info"
    _url_root = _http + _url_path

    def http_get(self, url):
        return super(JueShiTangMen, self).http_get(urljoin(self._url_root, url))

    def http_post(self, url, data):
        return super(JueShiTangMen, self).http_post(
                     urljoin(self._url_root, url), data)

    def parse_post(self, ctx):
        print("JueShiTangMen POST: " + ctx)
        raise MethodNotImpletion()

    def parse_get(self, ctx):
        print("JueShiTangMen GET: " + ctx)
        raise MethodNotImpletion()

    def _do_get_(self, tag_open, tag_close, ctx):
        assert tag_open != None and len(tag_open) > 0, "tag_open NULL"
        assert tag_close != None and len(tag_close) > 0, "tag_close NULL"
        assert ctx != None, "Content NULL"

        s = ctx.index(tag_open)
        e = ctx.index(tag_close, s + len(tag_open))

        return (ctx[s+len(tag_open):e], ctx[e+len(tag_close):])

    def get_href(self, ctx):

        try:
            val, *c = self._do_get_('href="', '"', ctx)
        except ValueError:
            return ""
        else:
            return val.strip()


class PageDownload(JueShiTangMen):
    def parse_post(self, ctx):
        if ctx.find(u'参数错误') == -1:

            val = ctx.replace("</p>", "\n\n").replace("<p>", "    ")
            self._info["content"] = re.sub("<[^>]*>", "", val)
        else:
            raise InvalidPageInfo("Invalid Page info {}".format(ctx))

        return self._info

    def parse_get(self, ctx):

        self._ctx = ctx.replace('\', '')
        self._info = {}
        info = self._info
        
        try:
            self._get_menu(info)
            self._get_title(info)
            self._get_content(info)

            self._get_next(info)
            self._get_prev(info)

        except Exception as e:
            raise
        else:
            return info

        return None


    def _do_get_data(self, tag_open, tag_close):
        val, self._ctx = self._do_get_(tag_open, tag_close, self._ctx)
        return val.strip()


    def _get_title(self, info):

        try:
            info["title"] = self._do_get_data('<h1>', '</h1>')
        except ValueError:
            raise TitleNotFound("title not found")


    def _get_content(self, info):
        try:
            val = self._do_get_data(
                    '<div style="padding: 40px 0px 0px 0px; width: 336px; float: right;"> </div><br>',
                    '<br>'
                    ).replace("</p>", "\n\n").replace("<p>", "    ")

            self._info["content"] = re.sub("<[^>]*>", "", val)

        except ValueError:
            raise PageContentNotFound("page content not found")


    def _get_prev(self, info):
        try:
            info["page_prev"] = self.get_href(
                    self._do_get_data('</strong> <a ', '</a>')
                )
        except ValueError:
            pass


    def _get_menu(self, info):
        try:
            info["menu"] = self.get_href(
                    self._do_get_data( '<div class="banner">' , '</div>')
                )
        except ValueError:
            pass

    def _get_next(self, info):
        try:
            info["page_next"] = self.get_href(
                    self._do_get_data('</strong> <a ', '</a>')
                )
        except ValueError:
            pass


class MenuDownload(JueShiTangMen):
    def parse_get(self, ctx):
        from collections import OrderedDict

        self._ctx = ctx
        self._items = OrderedDict()

        while self._ctx and len(self._ctx) > 0:
            self.get_next_item()

        if self._items:
            return self._items

        raise InvalidPageInfo("Invalid Menu Page")

    def get_next_item(self):

        try:
            val, self._ctx = self._do_get_(
                    '<li><span>', '</span></li>', self._ctx)
        except ValueError:
            self._ctx = None
            return None
        else:
            _k = re.sub("<[^>]*>", "", val)
            _v = self.get_href(val)
            self._items[_k] = _v.strip()



class BookInfoDownload(JueShiTangMen):
    def parse_get(self, ctx):
        print("BookInfo: " + ctx)
        return ctx

class URLDownload(url_download):
    pass

def _main():

    url = 'http://www.jueshitangmen.info/wudongqiankun/3.html'
    page = PageDownload()
    print(page.http_get(url))
#   page.http_post("http://localhost:8000/cgi-bin/hello.py", {"foo":"bar"})
#    menu = MenuDownload()
#    print(menu.http_get('http://www.jueshitangmen.info/wudongqiankun/'))
#   book = BookInfoDownload()
#   book.http_get('n/jiuzhuanhunchunjue/')


if __name__ == '__main__':
    _main()
