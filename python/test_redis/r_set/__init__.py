#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from r_testcase import Testcase
from r_testcasemodule import TestcaseModule

module_alias = 'set'

class TestcaseModuleSet(TestcaseModule):
    pass

class TestcaseSet(Testcase):
    pass

from .set_01 import *

