#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import enum
import string
import re

chunk_size = 5


CountingStatus = enum.Enum(
    value = 'CountingStatus',
    names = ('punctuation chinese english other')
)


chinese_re = re.compile(u'[\u4e00-\u9fa5]+', re.UNICODE)

def is_ascii(ch):
    return ch in string.ascii_letters or ch in string.digits


def is_chinese(ch):
    return chinese_re.search(ch)


def _output_show_line(ctx):

    for ch in ctx:
        if is_ascii(ch):
            print("{:>12}: {}".format("ASCII", ch))
        elif is_chinese(ch):
            print("{:>12}: {}".format("CHINESE", ch))
        else:
            print("{:>12}: {}".format("XXXXXX", ch))


if __name__ == '__main__':
    print("hello, test fast reading...")

    if len(sys.argv) <= 1:
        print("{} filename".format(sys.argv[0]))
        sys.exit(1)

    with open(sys.argv[1], 'r') as f:
        for n, line in enumerate(f):
            print("{:>3} {}".format(n, line), end='')
            _output_show_line(line)
            

