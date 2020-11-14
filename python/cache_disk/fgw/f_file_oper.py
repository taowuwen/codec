
import os
from f_event import FGWEventFactory, FGWEvent
from f_file import file_system

class FileStatRefresh:

    def __init__(self, name = 'file_refresh_stat', mkdir=0):

        self._name = name
        self._make = file_system.mkdir if mkdir else file_system.create

    def __str__(self):
        return self._name

    def __repr__(self):
        return str(self)

    def __call__(self, msg):

        print(f'{self}, msg: {msg}')
        disk, _fl, stat = msg.msg

        fn = file_system.find_file(_fl)
        if not fn:
            fn = self._make(_fl)

        fn.stat = stat

        if disk != fn.ext[msg.type.name]:

            if fn.ext[msg.type.name]:
                print(f'Warnning: {self} may exist')

            fn.ext[msg.type.name] = disk

file_refresh_file_stat = FileStatRefresh('file_refresh_file_stat')
file_refresh_dir_stat  = FileStatRefresh('file_refresh_dir_stat', 1)

def file_oper_register_all_event():

    file_oper_evts = {
        'refresh_file_stat': file_refresh_file_stat,
        'refresh_dir_stat':  file_refresh_dir_stat,
    }

    for evt, cb in file_oper_evts.items():
        FGWEventFactory().register(evt, cb)
