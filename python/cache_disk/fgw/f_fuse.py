
from __future__ import print_function, absolute_import, division

import logging
from f_file import file_system
from f_msg import FuseMsg
from f_event import FGWEvent
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

        msg = FuseMsg(fl, *args)
        _evt = FGWEvent(evt, msg)
        self._mq.put_msg(_evt)

        print(f'file oper: {evt}, event: {_evt}, file: {fl}, args: {args}, msg: {msg}')

        msg.wait()

        ret, msg = msg.result
        if ret != 0:
            raise FuseOSError(errno.EIO)

        return msg

    def do_oper(self, evt, path, *args):
        return self.do_file_oper(evt, file_system.find_file(path), *args)

    def chmod(self, path, mode):
        return self.do_oper('chmod', path, mode)

    def chown(self, path, uid, gid):
        return self.do_oper('chown', path, uid, gid)

    def create(self, path, mode):

        fl = file_system.find_file(path)
        if fl:
            raise FuseOSError(errno.EEXIST)
        else:
            fl = file_system.tmpfile(path, mode)

        self.do_file_oper('create', fl, mode)
        return fl.fd

    def getattr(self, path, fh=None):
        try:
            return file_system.find_file(path).stat
        except Exception:
            raise FuseOSError(errno.ENOENT)

    def mkdir(self, path, mode):
        fl = file_system.find_file(path)
        if not fl:
            fl = file_system.tmpdir(path, mode)
        else:
            raise FuseOSError(errno.EEXIST)

        self.do_file_oper('mkdir', fl, mode)

    def open(self, path, flags):
        fl = file_system.find_file(path)
        if not fl:
            fl = file_system.tmpfile(path, flags)

        self.do_file_oper('open', fl, flags)
        return fl.fd

    def read(self, path, size, offset, fh):
        print('read: {}'.format(fh))

        fl = file_system.find_file(path)
        return self.do_file_oper('read', fl, size, offset, fh)

    def readdir(self, path, fh):
        fl = file_system.find_file(path)
        if fl.is_dir():
            return ['.', '..'] + [key for key in fl.files.keys()]
        else:
            raise FuseOSError(errno.ENOTDIR)

    def readlink(self, path):
        raise FuseOSError(errno.EIO)

    def rename(self, old, new):
        self.do_oper('rename', old, new)

    def rmdir(self, path):
        self.do_oper('rmdir', path)

    def statfs(self, path):
        return file_system.statfs

    def symlink(self, target, source):
        raise FuseOSError(errno.EIO)
        self.do_oper('symlink', target, source)

    def truncate(self, path, length, fh=None):
        self.do_oper('truncate', path, length, fh)

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

    def flush(self, path, fip):
        self.do_oper('flush', path, fip)

    def release(self, path, fip):
        return self.do_oper('release', path, fip)

    def getxattr(self, path, name, position=0):
        return self.do_oper('getxattr', path, name, position)

    def listxattr(self, path):
        return self.do_oper('listxattr', path)

    def removexattr(self, path, name):
        return self.do_oper('removexattr', path, name)

    def setxattr(self, path, name, value, options, position=0):
        return self.do_oper('setxattr', path, name, value, options, position)

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
        self._th = None

    @property
    def mount(self):
        return self._mount

    @mount.setter
    def mount(self, val):
        self._mount = val

    def do_start(self, mount):
        if self._th and self._th.is_alive():
            return

        self.mount = mount
        self._th = FileFuseThread(self._fuse, self.mount)
        self._th.start()

    def do_stop(self):
        if self._th:
            self._th.do_stop()
            self._th.join()
            self._th = None


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
