#!/usr/bin/env python3
# -*- coding: utf-8 -*-

class Money:

    def __init__(self, name, val):
        self.name = name
        self.val  = val

    def __str__(self):
        return self.name

    def get_value(self):
        return self.val


Money.quarter = Money("Quarter", 25)
Money.dollar  = Money("Dollar", 100)


class MoneyList(list):

    def coins(self):
        return str([ str(a) for a in self ])

    def __str__(self):
        return str(int(self))

    def __int__(self):
        return sum([ a.get_value() for a in self ])

    def __iadd__(self, val):

        if isinstance(val, Money):
            _val = val.get_value()
        elif isinstance(val, int):
            _val = val
        else:
            return self

        _m = { 25: Money.quarter, 100: Money.dollar }.get(_val, None)

        if _m: 
            self.append(_m)

        return self

    def __isub__(self, val):

        _val = val

        if _val > int(self):
            return self

        while _val >= 25:

            if _val > 100:
                if Money.dollar in self:
                    self.remove(Money.dollar)
                elif Money.quarter in self:
                    [ self.remove(Money.quarter) for a in range(4) ]
                else:
                    assert 0, "error"

                _val -= 100
                continue

            if _val >= 25:

                if Money.quarter in self:
                    self.remove(Money.quarter)
                    _val -= 25
                else:
                    print("Could not make change")
                    return self

        return self



if __name__ == '__main__':

    a = MoneyList(map(lambda a: Money.quarter, range(10)))

    a += 25
    a += Money.dollar

    print(a)
    print(a.coins())

    b = MoneyList()
    print(b)

    b += 25
    print(b)

    a -= 75
    print(a)
    print(a.coins())
