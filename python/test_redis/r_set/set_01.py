#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from r_utils import logger, decode_str
from . import TestcaseSet

class SetTest_001(TestcaseSet):

    def prepare(self):
        super().prepare()

    def clean(self):
        self.delete(self.key)
        super().clean()

    def do_test_set_get(self):
        self.sadd(self.key, self.val)
        self.smembers(self.key)
