#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from r_testcase import Testcase
from r_testcasemodule import TestcaseModule

module_alias = 'connection'

class TestcaseModuleConnection(TestcaseModule):
    pass

class TestcaseConnection(Testcase):
    pass

from .connection_01 import *

