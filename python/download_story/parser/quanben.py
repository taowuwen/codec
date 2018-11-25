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


_url_path = "www.quanben5.com"
_url_root = _http + _url_path

class Quanben(HTTPDownload):

    def http_get(self, url):
        return super().http_get(urljoin(_url_root, url))

    def http_post(self, url, data):
        return super().http_post(urljoin(_url_root, url), data)

    def parse_post(self, ctx):
        print("Quanben POST: " + ctx)
        raise MethodNotImpletion()

    def parse_get(self, ctx):
        print("Quanben GET: " + ctx)
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


class PageDownload(Quanben):
    def parse_post(self, ctx):
        if ctx.find(u'参数错误') == -1:

            val = ctx.replace("</p>", "\n\n").replace("<p>", "    ")
            self._info["content"] = re.sub("<[^>]*>", "", val)
        else:
            raise InvalidPageInfo("Invalid Page info {}".format(ctx))

        return self._info

    def parse_get(self, ctx):

        self._ctx = ctx
        self._info = {}
        info = self._info
        
        try:
            self._get_title(info)
            self._get_content(info)
            self._get_script(info)
            self._get_prev(info)
            self._get_menu(info)
            self._get_next(info)

            if info["content"].rfind(u'如果显示不完整'):
                self.http_post(info["script_url"], info["script_data"])
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
            info["title"] = self._do_get_data('<h1 class="title1">', '</h1>')
        except ValueError:
            raise TitleNotFound("title not found")


    def _get_content(self, info):
        try:
            val = self._do_get_data(
                    '<div id="content">', '</div>'
                    ).replace("</p>", "\n\n").replace("<p>", "    ")

            self._info["content"] = re.sub("<[^>]*>", "", val)

        except ValueError:
            raise PageContentNotFound("page content not found")

    def _get_script(self, info):
        import time
        from collections import OrderedDict
        try:
            data = OrderedDict()
            data.update({ "_type":"ajax"})

            ajax = eval(self._do_get_data(
                    '<script type="text/javascript">ajax_post', 
                    '</script>'
                    ))

            data.update(OrderedDict(zip(ajax[2::2], ajax[3::2])))
            data.update({"rndval":round(time.time()*1000)})

            info["script_url"] = "/index.php?c={0}&a={1}".format(ajax[0], ajax[1])
            info["script_data"] = data

        except ValueError:
            pass


    def _get_prev(self, info):
        try:
            info["page_prev"] = self.get_href(
                    self._do_get_data('<p id="page_last"', '</p>')
                )
        except ValueError:
            pass


    def _get_menu(self, info):
        try:
            info["menu"] = self.get_href(
                    self._do_get_data( '<p id="page_dir"' , '</p>')
                )
        except ValueError:
            pass

    def _get_next(self, info):
        try:
            info["page_next"] = self.get_href(
                    self._do_get_data('<p id="page_next"' , '</p>')
                )
        except ValueError:
            pass


class MenuDownload(Quanben):
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
                    '<li class="c3">', '</li>', self._ctx)
        except ValueError:
            self._ctx = None
            return None
        else:
            _k = re.sub("<[^>]*>", "", val)
            _v = self.get_href(val)
            self._items[_k] = _v.strip()



class BookInfoDownload(Quanben):
    def parse_get(self, ctx):
        print("BookInfo: " + ctx)
        return ctx

class URLDownload(url_download):
    pass

def _main():

    url='/n/doupocangqiong/60.html'
    page = PageDownload()
    page.http_get(url)
#   page.http_post("http://localhost:8000/cgi-bin/hello.py", {"foo":"bar"})
#    menu = MenuDownload()
#    menu.http_get('/n/jiuzhuanhunchunjue/xiaoshuo.html')
#   book = BookInfoDownload()
#   book.http_get('n/jiuzhuanhunchunjue/')


if __name__ == '__main__':
    _main()
