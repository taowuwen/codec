#!/usr/bin/env python3
# -*- coding: utf-8 -*-  

import os, sys
import shutil
import urllib.parse
import urllib


def main():
    url = sys.stdin.read()
    print("{}".format(urllib.parse.unquote(url)))


if __name__ == '__main__':
    main()
