#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import random
import string



class MyArray:
    def __init__(self, x, y):

        self._ary = [ [ random.choice(string.ascii_letters) for _y in range(y)] for _x in range(x) ]

    def __str__(self):
        _s = ""
        for x in self._ary:
            _s += str(x) + "\n"
        return _s
        #return str(self._ary)

    def __len__(self):
        return len(self._ary)

    def __getitem__(self, idx):
        return self._ary[idx]

    def __setitem__(self, idx, val):
        self._ary[idx] = val

class WhatIWant:

    def do_it(self):
        print("do it")


class WhatIUse:
    def op(self, whatiwant):
        return whatiwant.do_it()

class AdapterTrans(WhatIWant):

    def __init__(self, myary):
        self.myary = myary

    def do_it(self):

        idx = 0
        res = {}

        while idx + 1 < len(self.myary):
            #print(self.myary[idx])
            #print(self.myary[idx+1])
            #print([x for x in zip(self.myary[idx], self.myary[idx+1])])

            res.update({
                x[0]:x[1] for x in zip(self.myary[idx], self.myary[idx+1]) 
            })

            idx += 2

        return res



if __name__ == '__main__':
    print("hello, testing for translate")

    ary = MyArray(6, 5)
    print(ary, len(ary))

    i_use = WhatIUse()
    adap = AdapterTrans(ary)
    print(i_use.op(adap))
