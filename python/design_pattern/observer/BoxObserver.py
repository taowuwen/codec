#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from Observer import *
import os
import sys


class ClickedEvent:

    def __init__(self, x, y):
        self.x = x
        self.y = y


class BoxObserver(Observer, Observable):

    def __init__(self, x, y, z):
        '''
        0  <= x <= 8
        30 <= y <= 37
        40 <= z <= 47
        '''

        self.x = x if 0 <= x <= 8 else 1
        self.y = y if 30 <= y <= 37 else 31
        self.z = z if 40 <= z <= 47 else 41

    def __eq__(self, other):

        if not type(self) is type(other):
            return False

        return self.x == other.x and self.y == other.y and self.z == other.z

    def __hash__(self):
        return hash(str(hash(self.x)) + str(hash(self.y)) + str(hash(self.z)))

    def __str__(self):
        s = "{:02};{};{}m".format(self.x, self.y, self.z)
        return "\33[0{self.x};{self.y};{self.z}m{:^20}\33[00m".format(s, self=self)

    def update(self, arg=None):
        pass

    def notify_servers(self, arg=None):
        pass



class BoxTable:
    def __init__(self, x, y):
        self._width = 20

        self.x = x
        self.y = y

        self.boxes = [[None for y in range(y) ] for x in range(x)]

        self.build_boxes()

    def build_boxes(self):

        for x in range(self.x):
            for y in range(self.y):
                if not self.boxes[x][y]:
                    self.boxes[x][y] = self.build_uniqe_box()

    def build_uniqe_box(self):

        import random

        x = random.randint(0,  9)
        y = random.randint(30, 38)
        z = random.randint(40, 48)

        bo = BoxObserver(x, y, z)

        return bo if not self.box_exist(bo) else self.build_uniqe_box()

    def box_exist(self, bo):

        for lst in self.boxes:
            if bo in lst:
                return True

        return False

    def _print_open(self):

        _s = ""

        for y in range(self.y):
            _s += '+' + self._width*'-'

        _s += "+\n"

        return _s


    def _print_close(self):
        return self._print_open()

    def _print_line(self, lst):

        _s = ""

        for item in lst:

            _s += "|" + str(item).center(self._width - 1)

        _s += "|\n"

        return _s


    def _print(self):
        from beautifultable import BeautifulTable

        table = BeautifulTable(default_padding=2)

        [ table.append_row(self.boxes[x]) for x in range(self.x) ]

        print(table)

    def _print_v1(self):
        print(str(self))

    _print = _print_v1


    def __str__(self):

        _s = ""

        for x in range(self.x):
            _s += self._print_open()
            _s += self._print_line(self.boxes[x])

        _s += self._print_close()

        return _s



if __name__ == '__main__':
    print("testing for color box clicking...")

    bo = BoxObserver(1, 31, 40)
    bo1 = BoxObserver(1, 31, 40)

    lst_bo = []
    lst_bo.append(bo)

    #sys.stdout.write(str(bo))
    print(hash(bo), id(bo))
    print(hash(bo1), id(bo1))
    print(bo, bo1 == bo, bo1 is bo)
    print(bo1 in lst_bo)
    
    #os.write(sys.stdout.fileno(), str(bo).encode() + '\n'.encode())

    bt = BoxTable(3, 5)
    print(bt)
