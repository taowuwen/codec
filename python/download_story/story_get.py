#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os

import tasks

import taskpool2


def use_pool2(*args):

    url, fl, *c = (args)

    with open(fl, "a+") as fp:
        tp = taskpool2.taskpool2(num=30, output=fp)
        tp.add_task(taskpool2.TaskPageDownload(url))
        tp.run()


def use_pool(*args):

    url, fl, *c = (args)

    tp = tasks.TaskPool(num = 20, fl=fl)
    tp.task_append(tasks.TaskPageDownload(url))
    tp.start()



# cmd url filename
if '__main__' == __name__:

    if len(sys.argv) < 2:
        print(" {} url ".format(sys.argv[0]))
        sys._exit(1)

    print("hello, story downloader, by taowuwen")
#    use_pool(*(sys.argv[1:]))
    use_pool2(*(sys.argv[1:]))
