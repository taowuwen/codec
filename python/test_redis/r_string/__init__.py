#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from r_testcase import Testcase
from r_testcasemodule import TestcaseModule

module_alias = 'string'

class TestcaseModuleString(TestcaseModule):
    pass

class TestcaseString(Testcase):
    pass

from .string_01 import *

