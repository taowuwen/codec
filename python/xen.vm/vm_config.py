#!/usr/bin/env python3
# -*- cofing: utf-8 -*-

from config import *
from vm_config_strategy import *
import argparse
import sys
import os
import traceback

def prepare_args():
    parser = argparse.ArgumentParser(description="This is VM configration control")

    # parser.add_argument('-i', action='store_true', default=False)
    parser.add_argument('-O', '--output', action='store', default=None)
    parser.add_argument('-C', '--config', action='store', default="xen.vm.startup.json")
    parser.add_argument('-d', '--delay',  action='store', default=20, type=int,  help='delay since the last vm startup')
    parser.add_argument('-p', '--pos',    action='store', default=-1, type=int, help='position that the vm ')
    parser.add_argument('-a', '--auto',   action='store_true', default=False, help='auto startup')
    parser.add_argument('-o', '--order',  action='store_true', default=False, help='ordered startup')
    parser.add_argument('-e', '--enable', action='store_true', default=False, help='enable vm startup')

    subparser = parser.add_subparsers(dest="action", help='supported cmds')

    # list 
    _list = subparser.add_parser('list', help='List current configrations')

    # create
    _create = subparser.add_parser('create', help='Create/reset config')

    # add
    _add = subparser.add_parser('add', help='Add vm startup info configration file')
    _add.add_argument('path', action='store', help='vm config file path')

    # del
    _del = subparser.add_parser('del', help='Add vm startup info configration file')
    _del.add_argument('path', action='store', help='vm config file path')

    # update
    _upt = subparser.add_parser('update', help='Add vm startup info configration file')
    _upt.add_argument('path', action='store', help='vm config file path')

    return parser


class NodeAlreadyExist(Exception): pass
class NodeNotExist(Exception): pass
class MethodNotFound(Exception): pass

class CtrlConfig:
    """
    load config strategy here
    """
    def __init__(self, args, parser):
        self._P = parser
        self._parser = args
        self._cfg    = {}
        self._strategy = []
        self._cfg_update = False

        self._strategy.insert(CfgStrategyOrderedStartup.pos,   CfgStrategyOrderedStartup())
        self._strategy.insert(CfgStrategyUnorderedStartup.pos, CfgStrategyUnorderedStartup())
        self._strategy.insert(CfgStrategyNoStartup.pos,        CfgStrategyNoStartup())

    def run(self, action):

        cmd = {
                'list': self._list,
                'create': self._create,
                'add': self._add,
                'del': self._del,
                'update': self._update
            }.get(action, self._default)

        if action is not "create":
            self._cfg = load_config(self._parser.config)
            if self._cfg:
                self._init_strategy()

        try:
            cmd()
        except (NodeAlreadyExist, NodeNotExist) as e:
            print("cmd: {action} failed, {e}".format(e=e, action=action))

        except Exception as e:
            print("cmd: {action} failed, {e}".format(e=e, action=action))
            traceback.print_exc()
        else:
            if self._cfg_update:
                self._build_cfg()
                out = self._parser.output if self._parser.output else self._parser.config
                write_config(self._cfg, out)
                self._cfg_update = False

    def _init_strategy(self):

        for strategy in self._strategy:
            for cfg in self._cfg[strategy.method]:
                strategy.add(CfgNode(*cfg), -1)


    def _search(self, node):
        for strategy in self._strategy:
            n = strategy.search(node)
            if n:
                return strategy, n

        return None, None


    def _default(self):
        self._P.print_help()

    def _list(self):
        for strategy in self._strategy:
            print("Strategy: {strategy.method}, total: {length}".format(
                strategy=strategy, length=len(strategy._nodes)))

            for n in strategy._nodes:
                print("\t {n}".format(n=n))

    def _build_cfg(self):
        for strategy in self._strategy:

            cfg = strategy.build_cfg()
            self._cfg.update(cfg)

        return self._cfg


    def _create(self):
        self._cfg_update = True

    def _add(self):
        node = CfgNode(
                self._parser.path, 
                self._parser.auto,
                self._parser.delay,
                self._parser.enable)

        a_strategy, a_n = self._search(node)

        if a_strategy:
            raise NodeAlreadyExist("{node} already exist".format(node=node))

        return self._do_add(node)

    def _do_add(self, node):

        if self._parser.auto:
            if self._parser.order:
                strategy = self._strategy[CfgStrategyOrderedStartup.pos]
            else:
                strategy = self._strategy[CfgStrategyUnorderedStartup.pos]
        else:
            strategy = self._strategy[CfgStrategyNoStartup.pos]

        strategy.add(node, self._parser.pos)
        self._cfg_update = True

        return True


    def _del(self):

        for strategy in self._strategy:
            n = strategy.delete(self._parser.path)
            if n:
                self._cfg_update = True
                return n

        raise NodeNotExist("Node for {self._parser.path} not exist".format(self=self))
        

    def _update(self):

        n = self._del()

        if not n:
            raise NodeNotExist("Node for {self._parser.path} not exist".format(self=self))

        n.autostartup = self._parser.auto
        n.delay = self._parser.delay
        n.enable = self._parser.enable

        return self._do_add(n)


if __name__ == '__main__':
    parser = prepare_args()

    args = parser.parse_args()

    ctrl = CtrlConfig(args, parser)

    ctrl.run(args.action)
