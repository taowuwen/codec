#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from r_testcase import Testcase
from r_testcasemodule import TestcaseModule

module_alias = 'pubsub'

class TestcaseModulePubSub(TestcaseModule):
    pass

class TestcasePubSub(Testcase):
    pass

from .pubsub_01 import *

