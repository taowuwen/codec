#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import string, sys


#sys.path += ['../statemachine']


from StateMachine import StateMachine
from MouseAction import MouseAction
from State import State



class Waitting(State):
    def run(self):
        print("Waitting: Broadcasting cheese smell")

    def next(self, input):
        if input == MouseAction.appears:
            return MouseTrap.luring

        return MouseTrap.waitting


class Luring(State):
    def run(self):
        print("Luring: Presenting cheese, door open")

    def next(self, input):

        if input == MouseAction.run_away:
            return MouseTrap.waitting

        if input == MouseAction.enters:
            return MouseTrap.trapping

        return MouseTrap.luring

class Trapping(State):
    def run(self):
        print("Trapping, closing door")

    def next(self, input):
        if input == MouseAction.escaps:
            return MouseTrap.waitting

        if input == MouseAction.trapped:
            return MouseTrap.holding

        return MouseTrap.trapping


class Holding(State):
    def run(self):
        print("Holding, Mouse caught")

    def next(self, input):
        
        if input == MouseAction.removed:
            return MouseTrap.waitting

        return MouseTrap.holding

class MouseTrap(StateMachine):
    def __init__(self):
        super().__init__(MouseTrap.waitting)


MouseTrap.waitting = Waitting()
MouseTrap.luring = Luring()
MouseTrap.trapping = Trapping()
MouseTrap.holding = Holding()


#moves = map(string.strip, open("MouseMoves.txt").readlines())

moves = [ a.strip() for a in open("MouseMove.txt").readlines() ]

MouseTrap().runAll(map(MouseAction, moves))
