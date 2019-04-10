#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import random
import time


class SwitchMode:
    longest_unused = 1
    least_use = 2


class Node:
    def __init__(self, k, v):
        self._k = k
        self._v = v
        self._freq = 0

    @property
    def freq(self):
        return self._freq

    @property
    def key(self):
        return self._k

    @key.setter
    def key(self, v):
        self._k = v

    @property
    def val(self):
        return self._v

    @val.setter
    def val(self, v):
        self._v = v

    def visited(self):
        self._freq = self._freq + 1

    def reset(self):
        self._k = -1
        self._v = -1
        self._freq = 0

    def __str__(self):
        return "({}, {}, {})".format(self.key, self.val, self._freq)


class CacheBuffer:
    def __init__(self):
        self._cache = []
        self._mode = 0

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, m):

        if self._mode is not m:
            self._mode = m
            self.sched()

    def create(self, total, mode):
        self._cache = [Node(-1, -1) for key in range(total)]
        self._mode = mode

        self._sched = {
            SwitchMode.longest_unused: self._sched_lonest_unused,
            SwitchMode.least_use: self._sched_least_use
        }

    def _sched_lonest_unused(self, n):

        if n:
            ind = self._cache.index(n)
            self._cache = self._cache[:ind] + self._cache[ind + 1:]
            self._cache.append(n)

    def _sched_least_use(self, n):
        def helper(_n):
            if isinstance(_n, Node):
                return _n.freq
            return _n

        self._cache.sort(key=helper)

    def sched(self, node=None):
        self._sched[self._mode](node)

    def destroy(self):
        self._cache = []

    def _find_node(self, k):

        for n in self._cache:
            if n.key == k:
                return n

        return None

    def _find_unused_node(self):
        for n in self._cache:
            if n.key <= 0:
                return n
        return None

    def _get_node_with_policy(self):

        n = self._find_unused_node()

        return n if n else self._cache[0]

    def __setitem__(self, k, v):

        n = self._find_node(k)

        if not n:
            n = self._get_node_with_policy()

            if n.key >= 0:
                print("DROP NODE: {}".format(n))
                n.reset()
            n.key = k

        n.val = v
        n.visited()
        self.sched(n)

    def __getitem__(self, k):

        n = self._find_node(k)
        if n:
            n.visited()
            self.sched(n)

        return n

    def __str__(self):
        return ", ".join([str(n) for n in self._cache])


def _do_test(buf):

    keys = [random.randint(1, 20) for a in range(50)]
    vals = [random.randint(50, 100) for a in range(50)]

    for k, v in zip(keys, vals):
        buf[k] = v
        print("buf[{}] = {}".format(k, buf[k]))

    print(buf)


if __name__ == '__main__':

    random.seed(int(time.time()))

    buf = CacheBuffer()
    buf.create(10, SwitchMode.longest_unused)

    _do_test(buf)
    buf.mode = SwitchMode.least_use
    _do_test(buf)

    buf.destroy()
