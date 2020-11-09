from f_event import FGWEventFactory, FGWEvent
from f_observer import FileObserver
from f_exception import *
from f_disk import DiskType

class FileRouter(FileObserver):
    '''
    1. active table
    2. default routes
    3. constant hash
    '''

    def __init__(self, queue):
        '''
        register event handler for all disk handle
        '''
        self.mq_fgw = queue
        super().__init__()
        self.reg_fuse_evt()
        self._cmd_table = {}
        self._hdd_disks = []
        self._mem_disks = []
        self._ssd_disks = []

    def update_unkown(self, *args, **kwargs):
        raise InvalidArgument(f'FR: invalid argument, {args} {kwargs}')

    def update(self, *args, **kwargs):
        print(f'FR recved update {args}, {kwargs}')

        cmd, *_ = args

        if not self._cmd_table:
            self._cmd_table = {
                'disk_add': self.handle_disk_add,
                'disk_del': self.handle_disk_del,
                'disk_upt': self.handle_disk_upt,
            }

        self._cmd_table.get(cmd, self.update_unkown)(*argv[1:], **kwargs)

    def disk_table(self, ty):
        if ty is DiskType.HDD:
            return self._hdd_disks

        if ty is DiskType.MEMORY:
            return self._mem_disks

        return self._ssd_disks

    def handle_disk_add(self, disk):

        dt = self.disk_table(disk.disk_type)
        if disk not in dt:
            dt.append(disk)

    def handle_disk_del(self, disk):
        dt = self.disk_table(disk.disk_type)
        if disk in dt:
            dt.remove(disk)

    def handle_disk_upt(self, disk):
        pass
        
    def reg_events(self, events = (), method = None):
        for evt in events:
            FGWEventFactory().register(evt, method)
         
    def reg_fuse_evt(self):
        evts = (
            'chmod', 'chown', 'create', 'mkdir', 'open',
            'read', 'rename', 'rmdir', 'symlink', 'readlink',
            'truncate', 'unlink', 'utimens', 'write'
        )
        self.reg_events(evts, self.handle_fuse_evt)

    def handle_fuse_evt(self, msg):
        print(f'handle fuse msg: {msg.evt}: {msg}')


        msg.release()

    def reg_disk_evt(self):
        pass

    def handle_disk_evt(self, msg):
        pass

    def reg_memory_evt(self):
        pass

    def handle_memory_evt(self, msg):
        pass
