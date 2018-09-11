#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from Observer import *
import os
import sys
import random

class Event:

    def __init__(self, name):
        self.name = name

    def __eq__(self, other):
        return self.name == other.name

    def __str__(self):
        return self.name

    def __hash__(self):
        return hash(self.name)

Event.clicked = Event("clicked")

class ClickedEvent(Event):

    def __init__(self, x, y):
        self.x = x
        self.y = y
        super().__init__("clicked")

    def pos(self):
        return (self.x, self.y)

    def __str__(self):
        return super().__str__() + " {}, {}".format(self.x, self.y)

class EventList(list):

    def __str__(self):
        return str([str(i) for i in self])


class BoxObserver(Observer, Observable):

    def __init__(self, x, y, z, outer=None):
        '''
        0  <= x <= 8
        30 <= y <= 37
        40 <= z <= 47
        '''
        self.outer = outer

        self.x = x if 0 <= x <= 8 else 1
        self.y = y if 30 <= y <= 37 else 31
        self.z = z if 40 <= z <= 47 else 41
        super().__init__()

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
        print("{} got notified on {}".format(self, arg))
        evt = arg
        x, y = evt.pos()
        self.x = self.outer.boxes[x][y].x


    def notify_servers(self, arg=None):

        print("update box {}".format(self), end="\t")
        # update color
        bo = self.outer.build_uniqe_box()
        self.x = bo.x
        self.y = bo.y
        self.z = bo.z
        del bo

        print("current box {}, evt: {}".format(self, arg))

        self.set_changed()
        super().notify_servers(arg)
        self.clear_changed()
        
        print(self.outer)


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

        x = random.randint(0,  8)
        y = random.randint(30, 37)
        z = random.randint(40, 47)

        bo = BoxObserver(x, y, z, self)

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

    def run(self, evt):
        if evt == Event.clicked:

            x, y = evt.pos()

            box = self.boxes[x][y]
            box.notify_servers(arg=evt)

    def build_observers_for_box(self, box):

        for x in range(self.x):
            for y in range(self.y):

                b = self.boxes[x][y]
                if not b is box:
                    box.add_observer(b)


    def build_observers(self):
        
        for x in range(self.x):
            for y in range(self.y):
                self.build_observers_for_box(self.boxes[x][y])

    def remove_observers(self):
        [ [self.boxes[x][y].clear_observers() for y in range(self.y)] for x in range(self.x) ]



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

    row = 5 
    col = 5

    bt = BoxTable(row, col)
    print(bt)

    bt.build_observers()

    _r = [random.randint(0, row-1) for x in range(row * col)]
    _c = [random.randint(0, col-1) for y in range(row * col)]

    evts = [ ClickedEvent(*a) for a in zip(_r, _c) ] 
    print(EventList(evts))

    for evt in evts:
        bt.run(evt)

    bt.remove_observers()
