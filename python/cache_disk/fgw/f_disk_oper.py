
import glob
import os
from f_event import FGWEventFactory, FGWEvent
from f_disk import DiskType
from cache_disk import fuse_evts

def scan_path(disk, path = None):
    if not path:
        return

    for fl in glob.glob(path + '/*'):
        if os.path.isdir(fl):
            disk.queue.put_msg(FGWEvent('refresh_dir_stat', disk.create_msg(disk, disk.phy2fuse(fl), os.stat(fl))))
            scan_path(disk, fl)
        else:
            disk.queue.put_msg(FGWEvent('refresh_file_stat', disk.create_msg(disk, disk.phy2fuse(fl), os.stat(fl))))


def disk_scan(msg, *args, **kwargs):
    disk = msg.msg[0]
    scan_path(disk, disk.root)

def disk_oper(msg, *args, **kwargs):
    disk, *_ = args

    attr = msg.event.lstrip(f'{disk.disk_type.name}_')
    print(f'{msg.event}, {disk.disk_type.name}: {disk}->{attr}  handle msg {msg} ')

    fn = msg.msg[0]
    fl = disk.fuse2phy(fn.abs_path)

    try:
        ret = getattr(disk, attr)(msg, fl, *msg.msg[1:])

        if ret is None:
            msg.result = (0, os.stat(fl))
        else:
            msg.result = (0, ret)
    except Exception as e:
        msg.result = (-1, f'failed on {fl} {msg}')
    finally:
        disk.send_rsp_msg(msg)

def disk_oper_register_all_event():
    disk_oper_evts = {
        'disk_scan': disk_scan
    }

    for evt,cb in disk_oper_evts.items():
        FGWEventFactory().register(evt, cb)

    for ty in DiskType:
        for evt in fuse_evts:
            FGWEventFactory().register(f'{ty.name}_{evt}', disk_oper)

