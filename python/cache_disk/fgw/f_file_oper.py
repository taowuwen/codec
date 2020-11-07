
import os
from f_event import FGWEventFactory, FGWEvent
from f_file import file_system

def file_refresh_file_stat(msg):
    print(f'file_refresh_file_stat; msg: {msg}')
    disk, _fl, stat = msg.msg

    fn = file_system.find_file(_fl)
    if not fn:
        fn = file_system.create(_fl)

    fn.stat = stat
    if disk not in fn.ext['hdd']:
        fn.ext['hdd'].append(disk)

def file_refresh_dir_stat(msg):
    print(f'file_refresh_dir_stat, msg: {msg}')
    disk, _dir, stat = msg.msg
    fn = file_system.find_file(_dir)
    if not fn:
        fn = file_system.mkdir(_dir)

    fn.stat = stat

    if disk not in fn.ext['hdd']:
        fn.ext['hdd'].append(disk)


def file_oper_register_all_event():

    file_oper_evts = {
        'refresh_file_stat': file_refresh_file_stat,
        'refresh_dir_stat':  file_refresh_dir_stat,
    }

    for evt, cb in file_oper_evts.items():
        FGWEventFactory().register(evt, cb)
