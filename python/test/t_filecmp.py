#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import filecmp
import sys


if __name__ == '__main__':

    print(filecmp.cmp(*sys.argv[1:]))
    dc = filecmp.dircmp(*sys.argv[1:])
    dc.report()
    dc.report_full_closure()

