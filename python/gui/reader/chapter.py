#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import re

from context import Context


class Chapter:

    def __init__(self, text='', file=None):

        self.chapter = []
        self.update_chapter(text, file)
        self._cur = 0

    def update_chapter(self, text='', file=None):

        if file:
            return self.load_file(file)
        else:
            ctx = Context()
            ctx.update(text)
            self.chapter.append((u"Unknown", ctx))

    def load_file(self, file):

        re_chpater = re.compile(u'^第.*章.*$', re.UNICODE)

        ctx = Context()
        self.chapter.append((u"Unknown", ctx))

        with open(file, 'r') as f:

            for l in f:

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

        if not hasattr(self, '_total'):
            self._total = sum([ var[1].total for var in self.chapter ])

        return self._total

    @property
    def current_chapter(self):
        return self._cur

    @property
    def total_chapter(self):
        return len(self.chapter)


if __name__ == '__main__':

    chp = Chapter(text='hello, world', file=u'/home/tww/a.txt')

    for ind, v in enumerate(chp.chapters):
        print("{} ==> {}".format(ind, v))

    for ind, v in enumerate(chp.contents):
        print("{} ==> {}".format(ind, v))

    #print(chp.chapters)
    #print(chp.contents)
    print(chp.total)
    print(chp.current_chapter)
    print(chp.total_chapter)
