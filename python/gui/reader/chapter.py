#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import re

from context import Context


class Chapter:

    def __init__(self, text='', file=None, lines=None):

        self.chapter = []
        self.update_chapter(text, file=file, lines=lines)
        self._cur = 0

    def reset(self):
        del self.chapter[:]
        self._cur = 0

    def update_chapter(self, text='', file=None, lines=None):

        if file:
            with open(file, 'r') as f:
                self.load_file(f)
        elif lines:
            self.load_file(lines)
        else:
            ctx = Context()
            ctx.update(text)
            self.chapter.append((u"Unknown", ctx))

    def load_file(self, lines):

        re_chpater = re.compile(u'^第.*章.*$', re.UNICODE)

        ctx = Context()
        self.chapter.append((u"Unknown", ctx))

        for l in lines:

            if re_chpater.search(l):
                ctx = Context()
                self.chapter.append((l, ctx))
            else:
                ctx.update(l)


    @property
    def chapters(self):
        return [ var[0] for var in self.chapter ]

    @property
    def contents(self):
        return [ var[1].content for var in self.chapter ]

    @property
    def total(self):
        return sum([ var[1].total for var in self.chapter ])

    @property
    def current_chapter(self):

        if self._cur >= self.total_chapter:
            return self._cur, ""

        return self._cur + 1, self.chapter[self._cur][0]

    @property
    def total_chapter(self):
        return len(self.chapter)

    def next_chunk(self, chunksize=5):

        if self._cur >= self.total_chapter:
            return 0, None

        ctx = self.chapter[self._cur]

        n, data = ctx[1].next_chunk(chunksize)

        if n > 0:
            return n, data

        self._cur += 1
        return self.next_chunk(chunksize)



if __name__ == '__main__':

    chp = Chapter(text='hello, world', file=u'/home/tww/b.txt')

    for ind, v in enumerate(chp.chapters):
        print("{} ==> {}".format(ind, v))

    for ind, v in enumerate(chp.contents):
        print("{} ==> {}".format(ind, v))

    #print(chp.chapters)
    #print(chp.contents)
    print(chp.total)
    print(chp.current_chapter)
    print(chp.total_chapter)

    while True:
        n, data = chp.next_chunk(10)

        print(n, data)

        if n <= 0:
            break
