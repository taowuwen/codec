#!/usr/bin/env python3
# -*- coding: utf-8 -*-


from r_container import TestCaseContainer
from r_utils import logger

class BuildProductFailed(Exception): pass

class Factory(TestCaseContainer):

    def create(self, key, **config):

        _product = self.get(key, None)

        if _product:
            return _product(**config)

        raise BuildProductFailed(f"Create '{key}' failed")


class ModuleBuildFactory(Factory):

    def build(self, key, **config):
        try:
            return self.create(key, **config)
        except BuildProductFailed as e:
            logger.error(f"Module build failed:  {e}")
            raise e


class TestcaseBuildFactory(Factory):
    def build(self, key, **config):
        try:
            return self.create(key, **config)
        except BuildProductFailed as e:
            logger.error(f"Testcase build failed: {e}")
            raise e


class DoTestRegister:

    def __init__(self, a = 2):
        self._a = a

    def __str__(self):
        return f"{self._a}"


if __name__ == '__main__':

    mbf = ModuleBuildFactory()

    config = {
        "a": 5
    }

    mbf.register('int', int)
    mbf.register('str', str)
    mbf.register('test', DoTestRegister)

    t = mbf.build('test', **config)
    logger.info(f"t = {t}")
    

    tbf = TestcaseBuildFactory()
    tbf.register('int', int)
    tbf.register('str', str)
    tbf.register('test', DoTestRegister)

    t = tbf.build('test', **config)
    logger.info(f"t = {t}")


