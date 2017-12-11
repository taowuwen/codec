#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os

import tasks


def main(*args):

    print("hello, story downloader, by taowuwen")

    url, fl, *c = (args)

    tp = tasks.TaskPool(num = 20, fl=fl)
    tp.task_append(tasks.TaskPageDownload(url))
    tp.start()



# cmd url filename
if '__main__' == __name__:

    if len(sys.argv) < 2:
        print(" {} url ".format(sys.argv[0]))
        sys._exit(1)

    main(*(sys.argv[1:]))
