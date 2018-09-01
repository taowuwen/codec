#!/usr/bin/env python3
# -*- coding: utf-8 -*-



class State:

    def __init__(self, val):
        self.value = val

    def __str__(self):
        return self.value


State.quiescent  = State("quiescent")
State.collecting = State("collecting")
State.selecting  = State("selecting")
State.more       = State("more")
#State.quit       = State("quit")
State.unaviliable = State("unaviliable")
State.nochange   = State("nochange")
State.makechange = State("make change")




