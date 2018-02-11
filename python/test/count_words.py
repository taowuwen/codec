#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import sys
import os
import re

from collections import defaultdict
from collections import Counter


words = defaultdict(lambda: 0)

def update_words(ln = None):

    global words

    if not ln:
        return

    for ind, word in enumerate(ln):
        ln[ind] = re.sub('^\'|\'$', '', word)

    l_words = Counter(ln)

    for w in l_words:
        words[w] += l_words[w]


def do_counting(fobj = sys.stdin):
    import re

    for ln in fobj:
        update_words(re.sub('[^a-zA-Z \t\']', '', ln.lower()).strip().split())

def print_top(n = 10):

    global words

    sorted_words = sorted(words, key=(lambda k: words[k]), reverse=True)

    if len(sorted_words) < n:
        n = len(sorted_words)

    print("Top {}".format(n))
    for i in range(n):
        w = sorted_words[i]
        print("{:>20} {:>8}".format(w, words[w]))

    



def do_print_st():
    global words

    print("total words {}".format(len(words)))

    print_top(10)


if __name__ == '__main__':

    if len(sys.argv) > 1:
        with open(sys.argv[1], 'r') as fobj:
            do_counting(fobj)
    else:
        do_counting(sys.stdin)

    do_print_st()
