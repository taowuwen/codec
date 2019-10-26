#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from r_testcase import Testcase
from r_testcasemodule import TestcaseModule

module_alias = 'server'

class TestcaseModuleServer(TestcaseModule):
    pass

class TestcaseServer(Testcase):
    pass

from .server_01 import *

