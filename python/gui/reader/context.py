#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
from line import Line

class Context:

    def __init__(self, fl = None):

        self._total = 0
        self._cur   = 0
        self._pos   = 0
        self._ctx   = []

        if fl:
            with open(fl, 'r') as f:
                for ln in f:
                    self.update(ln.strip())

    def _counting_line(self, ln):

        for ch in ln:
            print(self._total, ch)
            if is_english(ch) or is_digits(ch) or is_chinese(ch):
                self._total += 1

    def update(self, ln):
            l = Line(ln)

            self._ctx.append(l)
            self._total += l.total


    def reset(self):
        self._total = 0
        self._cur   = 0
        self._pos   = 0

        self._ctx   = []


    @property
    def content(self):
        return os.linesep.join([str(l) for l in self._ctx ])

    @property
    def total(self):
        return self._total

    @property
    def current(self):
        return self._pos

    @property
    def current_paragraph(self):
        return self._cur

    @property
    def total_paragraph(self):
        return len(self._ctx)

    def next_chunk(self, chunksize=5):

        if self._cur >= self.total_paragraph:
            return 0, None

        ln = self._ctx[self._cur]
        
        n, data = ln.get_n(chunksize)
        if n < chunksize:
            self._cur += 1

        if n > 0:
            self._pos += n
            return n, data

        return self.next_chunk(chunksize)


if __name__ == '__main__':

    ctx = Context("/home/tww/stories/tmp.txt")
#    ctx = Context()

#    ctx.update('hello, world, 是我了啦')
#    ctx.update(u'hello, world, 是我了啦------new')
    print("CONTENT: {}".format(ctx.content))
    print("total: ", ctx.total)
    print("current: ", ctx.current)
    print("total line: ", ctx.total_paragraph)
    print("current line: ", ctx.current_paragraph)

    while True:

        n, data = ctx.next_chunk(3)
        if not data:
            break

        print("{:>8d} / {:<8d} > {:>3}% {:>10}: {:<30s} ".format(ctx.current, ctx.total, int(ctx.current*100/ctx.total), n, data))


    print("total: ", ctx.total)
    print("current: ", ctx.current)
    print("total line: ", ctx.total_paragraph)
    print("current line: ", ctx.current_paragraph)
