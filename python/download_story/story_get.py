#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os

import tasks

import taskpool2

import argparse


class InvalidArgs(Exception):
    pass


def use_pool2():

    try:
        parse_arguments()
    except InvalidArgs as e:
        print("Exception Error: \r\n\r\n{}".format(e))
        sys.exit(-1)

    with open(args.fl, "a+") as fp:
        tp = taskpool2.taskpool2(num=30, output=fp, retry=args.retry)
        tp.add_task(taskpool2.TaskUrlDownload(args.url))
        tp.run()


def use_pool():

    try:
        parse_arguments()
    except InvalidArgs as e:
        print("Exception Error: \r\n\r\n{}".format(e))
        sys.exit(-1)

    tp = tasks.TaskPool(num = 20, fl=args.fl)
    tp.task_append(tasks.TaskPageDownload(args.url))
    tp.start()


def parse_arguments():

    desc="""
    Story downloader, by taowuwen@gmail.com version 1.0
    """
    parser = argparse.ArgumentParser(description=desc)

    parser.add_argument('-a', '--url', action='store', dest='url')
    parser.add_argument('-o', '--output', action='store', dest='fl', default="/tmp/story.txt")
    parser.add_argument('-r', '--retry', action='store', dest='retry', default=10, type=int)
    parser.add_argument('-v', '--version', action='version', version="%(prog)s 1.0")

    global args

    args = parser.parse_args(sys.argv[1:])
    if not args.url:
        parser.print_help()
        raise InvalidArgs("ERROR, Download URL should be supplied")

# cmd url filename
if '__main__' == __name__:
#    use_pool()
    use_pool2()
