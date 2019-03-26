#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from ep import timeit
from ep import show_item_info
import sys


def _index_words(text):

    result = []

    if text:
        result.append(0)

    for index, l in enumerate(text):
        if l == ' ':
            result.append(index + 1)
    return result


@show_item_info
@timeit
def _main():

    text = "Four score and seven years ago..."
    res =  _index_words(text)

    print(res)

if __name__ == '__main__':
    _main()
