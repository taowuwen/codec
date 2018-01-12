#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import string
import re
import enum
import collections

re_chinese = re.compile(u'[\u4e00-\u9fa5]+', re.UNICODE)

def is_english(ch):
    return ch in string.ascii_letters

def is_digits(ch):
    return ch in string.digits

def is_chinese(ch):
    return re_chinese.search(ch)


CountStatus = enum.Enum(
    value='CountStatus',
    names=('punctuation chinese english')
)


st_count = {
        CountStatus.punctuation: {
            CountStatus.punctuation: 0,
            CountStatus.chinese: 1,
            CountStatus.english: 1
            },
        CountStatus.chinese: {
            CountStatus.punctuation: 0,
            CountStatus.chinese: 1,
            CountStatus.english: 1
            },
        CountStatus.english: {
            CountStatus.punctuation: 0,
            CountStatus.chinese: 1,
            CountStatus.english: 0
            }
    }



class Line:

    def __init__(self, ctx):
        self._ctx   = ctx
        self._total = 0
        self._pos   = 0

        self._do_counting()

    def get_next_status(self, ch):

        if is_english(ch) or is_digits(ch):
            return CountStatus.english

        if is_chinese(ch):
            return CountStatus.chinese

        return CountStatus.punctuation

    def _do_counting(self):

        src = CountStatus.punctuation
        dst = CountStatus.punctuation

        for ch in self._ctx:

            dst = self.get_next_status(ch)

            self._total += st_count[src][dst]

            src = dst

    def __str__(self):
        return "{}".format(self._ctx)

    @property
    def total(self):
        return self._total

    @property
    def current(self):
        return  self._pos

    def get_n(self, chunk=3):

        data = ""
        n = 0

        src = CountStatus.punctuation
        dst = CountStatus.punctuation

        s = self._pos
        for ch in self._ctx[s:]:

            self._pos += 1

            dst = self.get_next_status(ch)
            n += st_count[src][dst]
            src = dst

            if n > chunk:
                self._pos -= 1                       # go back one
                n -= 1                               # count goes back
                break

            data += ch

        return n, data


if __name__ == '__main__':
    ln = Line(u'hello, world, 123 432 6782913    你好？？？???世界')

    print("content : {}".format(ln))
    print("words: {}".format(ln.total))
    print("current: {}".format(ln.current))
    print(ln.get_n(5))
    print(ln.get_n())
    print(ln.get_n())
    print(ln.get_n())
    print(ln.get_n())
