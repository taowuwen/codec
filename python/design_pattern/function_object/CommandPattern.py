#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Command: pass

class Loony(Command):
    def execute(self):
        print("You are a loony")


class NewBrain(Command):
    def execute(self):
        print("You need a new brain")

class Afford(Command):
    def execute(self):
        print("I could't afford a new brain")

class Macro:

    def __init__(self):
        self.cmd = []

    def add_cmd(self, cmd):
        self.cmd.append(cmd)

    def run(self):
        for cmd in self.cmd:
            cmd.execute()

if __name__ == '__main__':
    print("Command Pattern testing...")

    m = Macro()

    m.add_cmd(Loony())
    m.add_cmd(NewBrain())
    m.add_cmd(Afford())
    m.run()
