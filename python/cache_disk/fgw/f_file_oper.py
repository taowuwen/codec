
import os
from f_event import FGWEventFactory, FGWEvent
from f_file import file_system
from cache_disk import *

class FileStatRefresh:

    def __init__(self, name = 'file_refresh_stat', mkdir=0):

        self._name = name
        self._make = file_system.mkdir if mkdir else file_system.create

    def __str__(self):
        return self._name

    def __repr__(self):
        return str(self)

    def __call__(self, msg):

        logger.debug(f'{self}, msg: {msg} {msg.type.name} {msg.type}')
        disk, _fl, stat = msg.msg

        fn = file_system.find_file(_fl)
        if not fn:
            fn = self._make(_fl)

        fn.stat = stat

        if disk not in fn.ext[msg.type.name]:
            fn.ext[msg.type.name].append(disk)

        logger.debug(fn.ext.values(), any(fn.ext.values()))

file_refresh_file_stat = FileStatRefresh('file_refresh_file_stat')
file_refresh_dir_stat  = FileStatRefresh('file_refresh_dir_stat', 1)

def file_oper_register_all_event():

    file_oper_evts = {
        'refresh_file_stat': file_refresh_file_stat,
        'refresh_dir_stat':  file_refresh_dir_stat,
    }

    for evt, cb in file_oper_evts.items():
        FGWEventFactory().register(evt, cb)
