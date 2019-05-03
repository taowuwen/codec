#!/usr/bin/env python3
# -*- cofing: utf-8 -*-

from config import *
from vm_startup_strategy import *
import argparse
import sys
import os
import traceback

def prepare_args():
    parser = argparse.ArgumentParser(description="This is VM configration control")

    parser.add_argument('-C', '--config', action='store', default="xen.vm.startup.json")

    return parser.parse_args()


class NodeAlreadyExist(Exception): pass
class NodeNotExist(Exception): pass
class MethodNotFound(Exception): pass

class CtrlStartup:
    """
    Startup with config files
    """
    def __init__(self, parser):
        self._parser = parser
        self._cfg    = {}
        self._strategy = []

        self._strategy.insert(StartupStrategyOrderedStartup.pos,   StartupStrategyOrderedStartup())
        self._strategy.insert(StartupStrategyUnorderedStartup.pos, StartupStrategyUnorderedStartup())
        self._strategy.insert(StartupStrategyNoStartup.pos,        StartupStrategyNoStartup())

    def run(self):

        self._cfg = load_config(self._parser.config)
        if self._cfg:
            self._init_strategy()

        for strategy in self._strategy:
            strategy.startup()

    def _init_strategy(self):

        for strategy in self._strategy:
            for cfg in self._cfg[strategy.method]:
                strategy.add(StartupNode(*cfg), -1)


if __name__ == '__main__':
    parser = prepare_args()
    ctrl = CtrlStartup(parser)
    ctrl.run()
