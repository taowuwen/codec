#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import string, sys


#sys.path += ['../statemachine']


from StateMachine import StateMachine
from MouseAction import MouseAction


class StateT:
    def __init__(self):
        self.transitions = None

    def next(self, input, default=None):
        return self.transitions.get(input, default)


class Waitting(StateT):
    def run(self):
        print("Waitting: Broadcasting cheese smell")

    def next(self, input):

        print(input, MouseAction.appears)
        if not self.transitions:
            self.transitions = {
                MouseAction.appears: MouseTrap.luring
            }

        return super().next(input, MouseTrap.waitting)


class Luring(StateT):
    def run(self):
        print("Luring: Presenting cheese, door open")

    def next(self, input):

        if not self.transitions:

            self.transitions = {
                MouseAction.run_away: MouseTrap.waitting,
                MouseAction.enters: MouseTrap.trapping
            }

        return super().next(input, MouseTrap.luring)


class Trapping(StateT):
    def run(self):
        print("Trapping, closing door")

    def next(self, input):

        if not self.transitions:
            self.transitions = {
                    MouseAction.escaps: MouseTrap.waitting,
                    MouseAction.trapped: MouseTrap.holding
            }

        return super().next(input, MouseTrap.trapping)


class Holding(StateT):
    def run(self):
        print("Holding, Mouse caught")

    def next(self, input):

        if not self.transitions:
            self.transitions = {
                MouseAction.removed: MouseTrap.waitting
            }

        return super().next(input, MouseTrap.holding)


class MouseTrap(StateMachine):
    def __init__(self):
        super().__init__(MouseTrap.waitting)


MouseTrap.waitting = Waitting()
MouseTrap.luring = Luring()
MouseTrap.trapping = Trapping()
MouseTrap.holding = Holding()


#moves = map(string.strip, open("MouseMoves.txt").readlines())

moves = [ a.strip() for a in open("MouseMove.txt").readlines() if a.startswith("mouse")]

MouseTrap().runAll(map(MouseAction, moves))
