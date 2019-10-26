#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from r_container import TestCaseContainer
from r_utils import logger

class TestcaseModule:

    def __init__(self):
        self._testcase = None

    @property
    def testcase(self):
        return self._testcase

    @testcase.setter
    def testcase(self, v):
        self._testcase = v

    def prepare(self):
        pass

    def run(self, **config):

        for _tc in self.testcase.__subclasses__():
            tc = _tc(**config)

            logger.info(f"------------->>>>>>>>{tc}<<<<<<<<------------------")
            tc.prepare()
            tc.run()
            tc.clean()
            logger.info(f"---------------------{tc}------------------------")


    def clean(self):
        pass

    def __str__(self):
        return f'{self.__class__.__name__}'
