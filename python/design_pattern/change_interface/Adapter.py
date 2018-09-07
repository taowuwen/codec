#!/usr/bin/env python3
# -*- coding: utf-8 -*-

class WhatIHave:
    def g(self): print("whatihave g")
    def f(self): print("whatihave f")

class WhatIWant:
    def h(self): print("what i want is h")

class AdapterProxy(WhatIWant):
    def __init__(self, whatihave):
        self.whatihave = whatihave

    def h(self):
        self.whatihave.g()
        self.whatihave.f()

class WhatIUse:
    def op(self, whatIWant):
        whatIWant.h()


class WhatIUse2:
    def op(self, whatIhave):
        AdapterProxy(whatIhave).h()

class WhatIHave2(WhatIHave, WhatIWant):
    def h(self):
        self.g()
        self.f()

class WhatIHave3(WhatIHave):
    class InnerAdapter(WhatIWant):
        def __init__(self, outer):
            self.outer = outer

        def h(self):
            self.outer.g()
            self.outer.f()

    def whatIWant(self):
        return WhatIHave3.InnerAdapter(self)


if __name__ == '__main__':
    print("testing Adapter....")

    what_i_use = WhatIUse()
    what_i_have = WhatIHave()
    adapt = AdapterProxy(what_i_have)

    what_i_use.op(adapt)

    what_i_use_2 = WhatIUse2()
    what_i_use_2.op(what_i_have)

    what_i_have_2 = WhatIHave2()
    what_i_use_2.op(what_i_have_2)

    what_i_use.op(AdapterProxy(what_i_have_2))

    what_i_have_3 = WhatIHave3()
    what_i_use.op(what_i_have_3.whatIWant())




