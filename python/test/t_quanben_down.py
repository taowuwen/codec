#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import urllib
import sys

from urllib import request
from urllib.parse import urljoin
from urllib.parse import urlencode


class HttpDownload:
    pass


class QuanbenPageInfoParser:
    pass


def main():

    print("do download stories", sys.argv)
    if len(sys.argv) < 2:
        print("usage {} url output".format(sys.argv[0]))
        return 1


    url = sys.argv[1]
    fl  = "/tmp/tmp.txt"

    if len(sys.argv) > 2:
        fl = sys.argv[2]

    sys.stdout.write("hello, world\n")



if __name__ == '__main__':
    main()
