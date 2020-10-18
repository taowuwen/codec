
from __future__ import print_function, absolute_import, division

import logging
from f_file import file_system
import threading
import errno

try:
    from fuse import FUSE, FuseOSError, Operations, LoggingMixIn, fuse_exit
except ModuleNotFoundError:
    from fusepyng import FUSE, FuseOSError, Operations, LoggingMixIn, fuse_exit

class FileFuseMount(LoggingMixIn, Operations):

    def __init__(self, mq):
        self._mq = mq

    def do_file_oper(self, evt, fl, *args):

        msg = FuseMsg(fl, args)
        self._mq.put_msg(FGWEvent(evt, msg))

        print(f'file oper: {evt} ,{fl}, {args}, {msg}')

        return msg.wait()

    def do_oper(self, evt, path, *args):
        return self.do_file_oper(evt, file_system.find_file(path), *args)

    def chmod(self, path, mode):
        return self.do_oper('chmod', path, mode)

    def chown(self, path, uid, gid):
        return self.do_oper('chown', path, uid, gid)

    def create(self, path, mode):

        fl = file_system.create(path, mode)
        self.do_file_oper('create', fl, mode)
        return fl.fd

    def getattr(self, path, fh=None):
        return file_system.find_file(path).stat

    def mkdir(self, path, mode):
        fl = file_system.mkdir(path, mode)
        self.do_file_oper('mkdir', fl, mode)

    def open(self, path, flags):
        fl = file_system.find_file(path)
        self.do_file_oper('open', flags)
        return fl

    def read(self, path, size, offset, fh):
        print('read: {}'.format(fh))

        fl = file_system.find_file(path)
        self.do_file_oper('read', fl, size, offset, fh)


    def readdir(self, path, fh):
        fl = file_system.find_file(path)
        if fl.is_dir():
            return ['.', '..'] + fl.files.keys()
        else:
            raise FuseOSError(errno.ENOTDIR)

    def readlink(self, path):
        raise FuseOSError(errno.EIO)

    def rename(self, old, new):
        self.do_oper('rename', old, new)

    def rmdir(self, path):
        self.do_oper('rmdir', path)

    def statfs(self, path):
        return dict(f_bsize=512, f_blocks=4096, f_bavail=2048)

    def symlink(self, target, source):
        raise FuseOSError(errno.EIO)
        self.do_oper('symlink', target, source)

    def truncate(self, path, length, fh=None):
        return self.do_oper('truncate', path, length, fh)

    def unlink(self, path):
        return self.do_oper('unlink', path)

    def utimens(self, path, times=None):
        raise FuseOSError(errno.EIO)
        now = time()
        atime, mtime = times if times else (now, now)
        self.files[path]['st_atime'] = atime
        self.files[path]['st_mtime'] = mtime

    def write(self, path, data, offset, fh):
        return self.do_oper('write', path, data, offset, fh)

class FileFuseThread(threading.Thread):

    def __init__(self, fuse, mount, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._fuse = fuse
        self._mount = mount

    def run(self):
        logging.basicConfig(level=logging.DEBUG)
        print('fuse started, mount: {}'.format(self._mount))
        _f = FUSE(self._fuse, self._mount, foreground=True, allow_other=True)

    def do_stop(self):
        try:
            fuse_exit()
        except Exception as e:
            print('stop fuse exceptions {}'.format(e))
        self._stop()

class FileFuse:

    def __init__(self, queue, mount = '/tmp/fuse'):
        self.mq_fgw = queue
        self._fuse = FileFuseMount(queue)
        self._mount = mount

    @property
    def mount(self):
        return self._mount

    @mount.setter
    def mount(self, val):
        self._mount = val

    def do_start(self, mount):
        self.mount = mount
        self._th = FileFuseThread(self._fuse, self.mount)
        self._th.start()

    def do_stop(self):
        self._th.do_stop()
        self._th.join()

_gfuse = None

def f_fuse_init(queue):
    global _gfuse
    _gfuse = FileFuse(queue)

def f_fuse_start(mount):
    global _gfuse
    _gfuse.do_start(mount)

def f_fuse_stop():
    global _gfuse
    _gfuse.do_stop()
