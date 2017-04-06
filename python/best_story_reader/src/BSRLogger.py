#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import logging
import sys


fmt = "%(name)s %(asctime)s- %(levelname).4s--[%(filename)s:%(lineno).3s]: %(message)s"
logging.basicConfig(stream=sys.stdout, format=fmt, level=logging.DEBUG)

BSRlog = logging.getLogger("BSR")

log_debug = BSRlog.debug
log_info  = BSRlog.info
log_warn  = BSRlog.warning
log_crit  = BSRlog.critical
log_fatal = log_crit
