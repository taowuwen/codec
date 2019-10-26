#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from r_utils import logger, decode_str
from . import TestcaseString

class StringTest_001(TestcaseString):

    def prepare(self):
        super().prepare()

    def clean(self):
        super().clean()

    def do_test_set_get(self):
        self.set(self.key, self.val)
        val = decode_str(self.get(self.key))

        logger.trace(f'{val}')
