
from __future__ import print_function, absolute_import, division

import logging
from f_file import file_system
import threading

try:
    from fuse import FUSE, FuseOSError, Operations, LoggingMixIn, fuse_exit
except ModuleNotFoundError:
    from fusepyng import FUSE, FuseOSError, Operations, LoggingMixIn, fuse_exit

class FileFuseMount(LoggingMixIn, Operations):

    def __init__(self, mq):
        self._mq = mq

    def do_file_oper(self, evt, path, *args):

        msg = FuseMsg(file_system.find_file(path), args)
        self._mq.put_msg(FGWEvent(evt, msg))

        print(f'file oper: {evt} , {path}, {args}, {msg}')

        return msg.wait_finished()

    def chmod(self, path, mode):
        return do_file_oper('chmod', path, mode)

    def chown(self, path, uid, gid):
        return do_file_oper('chown', path, uid, gid)

    def create(self, path, mode):

        fl = file_system.create(path, mode)
        msg = FuseMsg(fl, mode)
        self._mq.put_msg(FGWEvent('create', msg))

        msg.wait_finished()

        return fl.fd

    def getattr(self, path, fh=None):
        return file_system.find_file(path).stat

    def getxattr(self, path, name, position=0):
        attrs = self.files[path].get('attrs', {})

        try:
            return attrs[name]
        except KeyError:
            return ''       # Should return ENOATTR

    def listxattr(self, path):
        attrs = self.files[path].get('attrs', {})
        return attrs.keys()

    def mkdir(self, path, mode):
        self.files[path] = dict(
            st_mode=(S_IFDIR | mode),
            st_nlink=2,
            st_size=0,
            st_ctime=time(),
            st_mtime=time(),
            st_atime=time())

        self.files['/']['st_nlink'] += 1

    def open(self, path, flags):
        self.fd += 1
        return self.fd

    def read(self, path, size, offset, fh):
        return self.data[path][offset:offset + size]

    def readdir(self, path, fh):
        return ['.', '..'] + [x[1:] for x in self.files if x != '/']

    def readlink(self, path):
        return self.data[path]

    def removexattr(self, path, name):
        attrs = self.files[path].get('attrs', {})

        try:
            del attrs[name]
        except KeyError:
            pass        # Should return ENOATTR

    def rename(self, old, new):
        self.data[new] = self.data.pop(old)
        self.files[new] = self.files.pop(old)

    def rmdir(self, path):
        # with multiple level support, need to raise ENOTEMPTY if contains any files
        self.files.pop(path)
        self.files['/']['st_nlink'] -= 1

    def setxattr(self, path, name, value, options, position=0):
        # Ignore options
        attrs = self.files[path].setdefault('attrs', {})
        attrs[name] = value

    def statfs(self, path):
        return dict(f_bsize=512, f_blocks=4096, f_bavail=2048)

    def symlink(self, target, source):
        self.files[target] = dict(
            st_mode=(S_IFLNK | 0o777),
            st_nlink=1,
            st_size=len(source))

        self.data[target] = source

    def truncate(self, path, length, fh=None):
        pass

    def unlink(self, path):
        pass

    def utimens(self, path, times=None):
        now = time()
        atime, mtime = times if times else (now, now)
        self.files[path]['st_atime'] = atime
        self.files[path]['st_mtime'] = mtime

    def write(self, path, data, offset, fh):
        pass



class FileFuse(threading.Thread):

    def __init__(self, queue, mount, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.mq_fgw = queue
        self._fuse = FileFuseMount(queue)
        self._running = 0
        self._mount = mount

    @property
    def mount(self):
        return self._mount

    @mount.setter
    def mount(self, val):
        self._mount = val

    def run(self):
        logging.basicConfig(level=logging.DEBUG)
        self._running = 1
        _f = FUSE(self._fuse, self.mount, foreground=True, allow_other=True)
        self._running = 0

    @property
    def running(self):
        return self._running

    def do_stop(self):
        try:
            fuse_exit()
        except Exception as e:
            print(f'Fuse Stop error. {e}')
        
