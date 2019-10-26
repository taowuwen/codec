#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from r_testcase import Testcase
from r_testcasemodule import TestcaseModule

module_alias = 'hash'

class TestcaseModuleHash(TestcaseModule):
    pass

class TestcaseHash(Testcase):
    pass

from .hash_01 import *

