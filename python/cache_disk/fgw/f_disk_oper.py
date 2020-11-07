
import glob
import os
from f_event import FGWEventFactory, FGWEvent

def scan_path(disk, path = None):
    if not path:
        return

    for fl in glob.glob(path + '/*'):
        if os.path.isdir(fl):
            disk.queue.put_msg(FGWEvent('refresh_dir_stat', disk.create_msg(disk, disk.phy2fuse(fl), os.stat(fl))))
            scan_path(disk, fl)
        else:
            disk.queue.put_msg(FGWEvent('refresh_file_stat', disk.create_msg(disk, disk.phy2fuse(fl), os.stat(fl))))


def disk_scan(msg):
    disk = msg.msg[0]
    scan_path(disk, disk.root)

def disk_oper_register_all_event():
    disk_oper_evts = {
        'disk_scan': disk_scan
    }

    for evt,cb in disk_oper_evts.items():
        FGWEventFactory().register(evt, cb)
