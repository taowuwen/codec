#!/usr/bin/env python3
# -*- coding: utf-8 -*-  

import os, sys
import shutil
import urllib


def main():
    _l = int(os.environ.get('CONTENT_LENGTH', '0'))

    if _l == 0:
        return 0

    ln = sys.stdin.read(_l)
    print("{}".format(ln))


if __name__ == '__main__':
    main()
