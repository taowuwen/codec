#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import absolute_import

import parser.quanben as quanben
from parser.httpdownload import _http
from urllib.parse import urljoin
from parser.httpdownload import url_download
import re
import os

class Qb5:
    _url_path = 'www.qb5.io'
    _url_root = _http + _url_path


class PageDownload(Qb5, quanben.PageDownload):
    def _get_title(self, info):

        try:
            info["title"] = self._do_get_data('<h1>', '</h1>')
        except ValueError:
            raise quanben.TitleNotFound("title not found")


    def _get_content(self, info):
        try:
            val = self._do_get_data(
                    '<div class="articlebody" id="content">', '</div>'
                    ).replace("</p>", os.linesep * 2).replace("<p>", " "*8)

            self._info["content"] = re.sub("<[^>]*>", "", val)

        except ValueError:
            raise quanben.PageContentNotFound("page content not found")

    def _get_script(self, info):
        import time
        try:
            data = { "_type":"ajax", "rndval":round(time.time()*1000)}

            ajax = eval('(' + self._do_get_data('setTimeout("ajax_post(', ')') + ')')
            data.update(dict(zip(ajax[2::2], ajax[3::2])))

            info["script_url"] = "/index.php?c={0}&a={1}".format(ajax[0], ajax[1])
            info["script_data"] = data

        except ValueError:
            pass

    def _get_prev(self, info):
        try:
            info["page_prev"] = self.get_href(
                    self._do_get_data('<span>', '</span>')
                )
        except ValueError:
            pass


    def _get_menu(self, info):
        try:
            info["menu"] = self.get_href(
                    self._do_get_data('<span>', '</span>')
                )
        except ValueError:
            pass

    def _get_next(self, info):
        try:
            info["page_next"] = self.get_href(
                    self._do_get_data('<span>', '</span>')
                )
        except ValueError:
            pass

    
class MenuDownload(Qb5, quanben.MenuDownload):
    def get_next_item(self):

        try:
            val, self._ctx = self._do_get_(
                    '<li>', '</li>', self._ctx)
        except ValueError:
            self._ctx = None
            return None
        else:
            _k = re.sub("<[^>]*>", "", val)
            _v = self.get_href(val)
            self._items[_k] = _v.strip()

class URLDownload(url_download):
    pass

def _main():

#   with open('/tmp/du-14.html', 'r') as f:
#       page = PageDownload()
#
#       info = page.parse_get(f.read())
#
#       print(info)
#
#   return 0

        

    page = PageDownload()
    print(page._url_root)
    url = '/xs-14241/du-15.html'
    info = page.http_get(url)
    print(info)

#   page.http_post("http://localhost:8000/cgi-bin/hello.py", {"foo":"bar"})


#   with open('/tmp/index.html', 'r') as f:
#       menu = MenuDownload()
#       items = menu.parse_get(f.read())
#
#       if items:
#           for item in items:
#               print("{} -> {}".format(item, items[item]))
#
#   return 0

#   try:
#       menu = MenuDownload()
#       print(menu._url_path)
#       url = '/xs-14241/'
#       items = menu.http_get(url)
#
#   except Exception as e:
#       print(e)
#
#   else:
#       for item in items:
#           print("{} -> {}".format(item, items[item]))



if __name__ == '__main__':
    _main()
