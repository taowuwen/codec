#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from vm_common import Node, Strategy


class CfgNode(Node):
    pass


class CfgStrategy(Strategy):
    method = "default"
    pos = -1

    def delete(self, node):

        n = self.search(node)
        if n:
            self._nodes.remove(n)

        return n

    def search(self, node):

        for n in self._nodes:
            if n == node:
                return n

        return None

    def build_cfg(self):

        _cfg = []

        for n in self._nodes:

            _cfg.append((n._path, n._autostartup, n._delay, n._enable))

        return  {self.__class__.method: _cfg}
            


class CfgStrategyOrderedStartup(CfgStrategy):
    pos = 0
    method = "ordered"

class CfgStrategyUnorderedStartup(CfgStrategy):
    pos = 1
    method = "unordred"

class CfgStrategyNoStartup(CfgStrategy):
    pos = 2
    method = "disable"

