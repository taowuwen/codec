#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import sys
import os
from Items import ItemSlots, Item 

class Template:
    def __init__(self, val):
        self.val = val

    def __str__(self):
        return self.val


class State:

    def __init__(self, val):
        self.value = val

    def __str__(self):
        return self.value


State.quiescent  = State("quiescent")
State.collecting = State("collecting")
State.selecting  = State("selecting")
State.dispense   = State("dispense")
State.unavailiable = State("unavailiable")
State.nochange   = State("nochange")
State.makechange = State("make change")


def condition_none(a, b): return True
def condition_not_enough(a, b): return a < b
def condition_not_availiable(a, b): return a > 0


class MakeChange:
    def __init__(self, name):
        self.name = name

    def __str__(self):
        return self.name



class Condition:
    def __init__(self, val):
        self.val = val

    def __str__(self):
        return self.val

    def test(self, a, b):
        assert 0, "Not Implemention"


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


class Transition:
    def __init__(self, name):
        self.name = name

    def run(self, evt):
        assert 0, "Not Implemention"



class Quit(Template): pass

Quit.quiter = Quit("do quit....")

class SelectedItem:
    def __init__(self, val):
        self.selected = val

    def __str__(self):
        return self.selected

    def get_value(self):
        return self.selected

SelectedItem.select = SelectedItem("00")



class StateMachine:

    """
    {
        CurrentState: ([Condition, transition, next_state], [cond, transition, nextst]),
    }
    """
    def __init__(self, st = None):
        self._st = {}
        self.cur_st = st


    def next_state(self, evt):

        val = self._st.get((self.cur_st, evt.__class__))

        assert val, "not implemented"

        cond, trans, next_s = val

        if not cond or cond(evt):
            self.cur_st = next_s

            if trans: trans(evt)

            return True

        return False


class VendingMachine(StateMachine):

    def __init__(self, changes=100):

        self.change = changes
        self.item_slots = ItemSlots(4, 5)
        self.item_slots.load()

        self.total = 0

        super().__init__(State.quiescent)

        self.build_state_table()


    def collect_money(self, evt):

        val = int(evt.get_value())

        self.total += val
        self.change += val

        print("total: {}, change: {}".format(self.total, self.change))
        self.item_slots.show_items_v1()

    def make_change(self, evt):
        """
        we should upadte this into a real change maker, 
        we have only quarter and doller coin, so, we gonna change those only
        """
        print("do make change.... {}".format(self.total))
        if self.total > 0:
            assert self.change > self.total, "error happenned"
            self.change -= self.total
            self.total = 0
            self.cur_st = State.quiescent
            print("Done, {} changes left".format(self.change))

    def selected_item(self, evt):

        item = self.item_slots.get_item(evt.get_value())

        if not item:
            print("Product Not Exist, please rechoose...")
            return False

        if item.quantity <= 0:
            self.cur_st = State.unavailiable
            print(" {} unavailiable ".format(item))
            return False

        if item.price > self.total:
            self.cur_st = State.collecting
            print("{} money not enough, please insert coins".format(item))
            return False

        # dispense
        self.cur_st = State.dispense
        return self.do_dispense(evt)

    def do_dispense(self, evt):
        item = self.item_slots.get_item(evt.get_value())

        assert item, "error here"

        print("Dispensing item: {}".format(item))

        item.quantity -= 1
        self.total -= item.price

        self.item_slots.show_items_v1()
        print("left: {}, change: {}".format(self.total, self.change))
        print("Want More?")

    def build_state_table(self):

        # for Quiescent
        st = {
                (State.quiescent, Money.quarter.__class__): (None, self.collect_money, State.collecting),
                (State.quiescent, Quit.quiter.__class__):  (None, None, State.quiescent)
        }
        self._st.update(st)

        # for collecting
        st = {
            (State.collecting, Money.quarter.__class__): ( None, self.collect_money, State.collecting),
            (State.collecting, SelectedItem.select.__class__): (None, self.selected_item, State.selecting),
            (State.collecting, Quit.quiter.__class__): (None, self.make_change, State.makechange)
        }
        self._st.update(st)

        # for selecting
        st = {
            (State.selecting, Money.quarter.__class__): ( None, self.collect_money, State.collecting),
            (State.selecting, SelectedItem.select.__class__): (None, self.selected_item, State.selecting),
            (State.selecting, Quit.quiter.__class__): (None, self.make_change, State.makechange)
        }
        self._st.update(st)

        # for unavailiable
        st = {
            (State.unavailiable, Money.quarter.__class__): ( None, self.collect_money, State.collecting),
            (State.unavailiable, SelectedItem.select.__class__): (None, self.selected_item, State.selecting),
            (State.unavailiable, Quit.quiter.__class__): (None, self.make_change, State.makechange)
        }
        self._st.update(st)

        # for dispense
        st = {
            (State.dispense, Money.quarter.__class__): ( None, self.collect_money, State.collecting),
            (State.dispense, SelectedItem.select.__class__): (None, self.selected_item, State.selecting),
            (State.dispense, Quit.quiter.__class__): (None, self.make_change, State.makechange)
        }
        self._st.update(st)



if __name__ == '__main__':

    vm = VendingMachine()

    inputs = [ 
        Money.quarter,
        Money.quarter,
        Money.dollar,
        SelectedItem("34"),
        SelectedItem("00"),
        Quit.quiter
    ]

    for evt in inputs:
        vm.next_state(evt)


