#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Node:
    def __init__(self, path, autostartup, delay, enable):
        self._path = path
        self._autostartup = autostartup
        self._delay = delay
        self._enable = enable

    def __eq__(self, other):

        if isinstance(other, str):
            return other == self._path
        
        if isinstance(other, Node):
            return other._path == self._path

    def __str__(self):
        return '{self._path}, {self._autostartup}, {self._delay}, {self._enable}'.format(self=self)

    @property
    def autostartup(self):
        return self._autostartup

    @autostartup.setter
    def autostartup(self, v):
        self._autostartup = v

    @property
    def delay(self):
        return self._delay

    @delay.setter
    def delay(self, v):
        self._delay = v

    @property
    def enable(self):
        return self._enable

    @enable.setter
    def enable(self, v):
        self._enable = v


class Strategy:
    def __init__(self):
        self._nodes = []

    def add(self, node, pos=-1):

        if pos <= -1:
            self._nodes.append(node)
        else:
            self._nodes.insert(pos, node)


if __name__ == '__main__':
    class TestNode(Node):
        pass

    n = Node("/etc/aaa", False, 20, True)
    tn = TestNode("/etc/aaa", False, 20, True)
    c = "/etc/aaa"
    d = "/etc/bbb"
    tn1 = TestNode("/etc/bbb", False, 20, True)

    print(n, tn, c, d, tn1)
    print(n == tn)
    print(n == c)
    print(tn == c)
    print(tn == tn1)
    print(tn == d)
    print(tn1 == d)


