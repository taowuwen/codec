#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import redis
from r_utils import logger

class Testcase(redis.Redis):

    def prepare(self):
        self.key = f'{self}'
        self.val = self.key + "-" + self.key
        pass

    def run(self):

        for tc in dir(self):
            if tc.startswith('do_test_'):
                logger.info(f'>>>>>>>>>>> {tc}')
                testcase = getattr(self, tc)
                testcase()

    def clean(self):
        pass

    def __str__(self):
        return f'{self.__class__.__name__}'

