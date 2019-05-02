#!/usr/bin/env python
# -*- cofing: utf-8 -*-

from config import *
import argparse
import sys
import os


def prepare_args():
    parser = argparse.ArgumentParser(description="This is VM configration control")

    # parser.add_argument('-i', action='store_true', default=False)
    parser.add_argument('-O', '--output', action='store', dest='file_out', default=None)
    parser.add_argument('-C', '--config', action='store', dest='config', default="xen.vm.startup.json")
    parser.add_argument('-d', '--delay',  action='store', default=20, type=int,  help='delay since the last vm startup')
    parser.add_argument('-a', '--auto',   action='store_true', default=True, help='auto startup')
    parser.add_argument('-o', '--order',  action='store_true', default=False, help='ordered startup')
    parser.add_argument('-p', '--pos',    action='store', default=-1, type=int, help='position that the vm ')
    parser.add_argument('-e', '--enable', action='store_true', default=True, help='enable vm startup')

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

    # enable
    _enable = subparser.add_parser('enable', help='enable vm startup')
    _enable.add_argument('path', action='store', help='vm config file path')

    # disable
    _disable = subparser.add_parser('disable', help='disable vm startup')
    _disable.add_argument('path', action='store', help='vm config file path')

    return parser.parse_args()


class CtrlConfig:
    """
    load config strategy here
    """
    def __init__(self, config, delay=20, file_out=None, enable=True, order=False, pos=-1):

        self._path   = config
        self._delay  = delay
        self._fl_out = file_out if file_out else self._path
        self._enable = enable
        self._order  = order
        self._pos    = pos
        self._cfg    = {}


    def run(self, action):

        cmd = {
                'list': self._list,
                'create': self._create,
                'add': self._add,
                'del': self._del,
                'update': self._update,
                'enable': self._enable,
                'disable': self._enable
            }.get(action, self._default)

        # create cfg strategy node

        if action is not "create":
            self._cfg = load_config(self._path)

        try:
            cmd()
        except:
            print(f"cmd {action} failed")
        else:
            write_config(self._cfg, self._fl_out)

    def _default(self):
        print("should never show up this line")

    def _list(self):
        print("Not implementation...")
        pass

    def _create(self):
        print("Not implementation...")
        # update with cfg strategy
        pass

    def _add(self):
        print("Not implementation...")
        pass

    def _del(self):
        print("Not implementation...")
        pass

    def _update(self):
        print("Not implementation...")
        pass

    def _enable(self):
        print("Not implementation...")
        pass



if __name__ == '__main__':
    parser = prepare_args()
    print(parser, parser.action)

    ctrl = CtrlConfig(
                parser.config,
                delay  = parser.delay,
                file_out = parser.file_out,
                enable = parser.enable,
                order = parser.order,
                pos = parser.pos
            )

    ctrl.run(parser.action)

