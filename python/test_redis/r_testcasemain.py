#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import glob

from r_utils import logger, DebugLevel
from r_testcase import Testcase
from r_testcasemodule import TestcaseModule
from r_factory import ModuleBuildFactory, TestcaseBuildFactory, BuildProductFailed
from r_container import TestCaseContainer

class ModuleNotFound(Exception): pass

class TestcaseMain:

    def __init__(self):

        self.module_dict = {}
        self.module_build_factory = None
        self.modules = None

    def run(self, modules={}, **config):

        try:
            self._scan_modules()
            self._create_module_build_factory(modules)
            self._module_register(modules)
            self._module_run(**config['redis'])
        except Exception as e:
            logger.error(f'{type(e)}: {e}')

        return 0

    def _scan_modules(self):

        for package in glob.glob('*/'):

            if not package.startswith('r_'):
                continue

            try:
                pkg = __import__(package.strip('/'))

                if not hasattr(pkg, "module_alias"):
                    continue

                alias = pkg.module_alias
                _cls_mod = None
                _cls_testcase = None

                for _cls in dir(pkg):

                    attr = getattr(pkg, _cls)

                    if attr in Testcase.__subclasses__():
                        _cls_testcase = _cls

                    if attr in TestcaseModule.__subclasses__():
                        _cls_mod = _cls

                    if _cls_testcase and _cls_mod:
                        break

                self.module_dict[alias] = [pkg, f'{_cls_mod}', f'{_cls_testcase}']

            except Exception as e:
                logger.warn(f'import error {e}, {type(e)}')

    def _create_module_build_factory(self, modules = {}):
        self.module_build_factory = ModuleBuildFactory()

        for _mod in modules:

            mod = self.module_dict.get(_mod, None)

            if not mod:
                raise ModuleNotFound(f"module '{_mod}' not found, current supported modules {list(self.module_dict.keys())}")

            self.module_build_factory.register(_mod, getattr(mod[0], mod[1]))


    def _module_register(self, modules={}):
        self.modules = TestCaseContainer()

        for _mod in modules:
            logger.trace(f'register module  {_mod}')
            self.modules.register(_mod, self.module_build_factory.build(_mod))

    def _module_run(self, **config):

        if not self.modules:
            return 1

        for _mod in self.modules:

            mod_map = self.module_dict.get(_mod)
            mod = self.modules[_mod]

            logger.notice("")
            logger.notice(f"=============={mod}===============")

            mod.testcase = getattr(mod_map[0], mod_map[2])
            mod.prepare()
            mod.run(**config)
            mod.clean()


if __name__ == '__main__':

    modules = {"string", "set", "server", "hash", "connection", "pubsub"}

    config = {
        "modules": modules,
        "redis": {
            "db" : 15,
            "host": "localhost",
            "port": "6379",
        }
    }

    logger.level = DebugLevel.dbg

    sys.exit(TestcaseMain().run(**config))
