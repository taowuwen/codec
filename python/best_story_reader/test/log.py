#!/usr/bin/env python3
#-*- coding: utf-8 -*-


import logging
import sys


#logging.basicConfig(filename='example.log',level=logging.DEBUG)

"""
|  %(name)s            Name of the logger (logging channel)
|  %(levelno)s         Numeric logging level for the message (DEBUG, INFO,
|                      WARNING, ERROR, CRITICAL)
|  %(levelname)s       Text logging level for the message ("DEBUG", "INFO",
|                      "WARNING", "ERROR", "CRITICAL")
|  %(pathname)s        Full pathname of the source file where the logging
|                      call was issued (if available)
|  %(filename)s        Filename portion of pathname
|  %(module)s          Module (name portion of filename)
|  %(lineno)d          Source line number where the logging call was issued
|                      (if available)
|  %(funcName)s        Function name
|  %(created)f         Time when the LogRecord was created (time.time()
|                      return value)
|  %(asctime)s         Textual time when the LogRecord was created
|  %(msecs)d           Millisecond portion of the creation time
|  %(relativeCreated)d Time in milliseconds when the LogRecord was created,
|                      relative to the time the logging module was loaded
|                      (typically at application startup time)
|  %(thread)d          Thread ID (if available)
|  %(threadName)s      Thread name (if available)
|  %(process)d         Process ID (if available)
|  %(message)s         The result of record.getMessage(), computed just as
|                      the record is emitted

"""

fmt = "%(name)s %(asctime)s- %(levelname).4s--[%(funcName)s: %(lineno)s]:\t %(message)s"

logging.basicConfig(stream=sys.stdout, format=fmt, level=logging.DEBUG)
logging.debug('This message should go to the log file')
logging.info('So should this')
logging.warning('And this, too')

logging.warning("warning msg")
logging.warn("warning msg")
logging.info("info msg")


logger = logging.getLogger(__name__)

def update_logger():
	global logger
	logger.debug('This message should go to the log file')
	logger.info('So should this')
	logger.warning('And this, too')

update_logger()


# create logger
logger = logging.getLogger('simple_example')
logger.setLevel(logging.DEBUG)

# create console handler and set level to debug
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)

# create formatter
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')

# add formatter to ch
ch.setFormatter(formatter)

# add ch to logger
logger.addHandler(ch)

# 'application' code
logger.debug('debug message')
logger.info('info message')
logger.warn('warn message')
logger.error('error message')
