from f_event import FGWEventFactory, FGWEvent
from f_observer import FileObserver
from f_exception import *
from f_disk import DiskType
from cache_disk import fuse_evts
from f_msg import *

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
        self.reg_disk_evt()

        self._cmd_table = {}
        self._hdd_disks = []
        self._mem_disks = []
        self._ssd_disks = []

        self._current_memory = 0
        self._current_hdd    = 0
        self._current_ssd    = 0

    def _find_memory_node(self):
        if len(self._mem_disks) > 0:
            ret = self._mem_disks[self._current_memory]
            self._current_memory = (self._current_memory + 1) % len(self._mem_disks)
            return ret

        else:
            return None

    def _find_disk_node(self):
        if len(self._hdd_disks) > 0:
            ret = self._hdd_disks[self._current_hdd]
            self._current_hdd = (self._current_hdd + 1) % len(self._hdd_disks)
            return ret
        else:
            return None

    def _find_ssd_node(self):
        if len(self._ssd_disks) > 0:
            ret = self._ssd_disks[self._current_ssd]
            self._current_ssd = (self._current_ssd + 1) % len(self._current_ssd)
            return ret
        else:
            return None


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

        self._cmd_table.get(cmd, self.update_unkown)(*args[1:], **kwargs)

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

        # reset disk current pos
        self._current_memory = 0
        self._current_hdd    = 0
        self._current_ssd    = 0

    def handle_disk_upt(self, disk):
        pass
        
    def reg_events(self, events = (), method = None):
        for evt in events:
            FGWEventFactory().register(evt, method)
         
    def reg_fuse_evt(self):
        self.reg_events(fuse_evts, self.handle_fuse_evt)

    def handle_fuse_evt(self, msg):
        '''
            a. file/dir not exist
            b. file/dir do exist

            a. file/dir not exist? (msg evt is int (open, mkdir))
                step1. 1. send msg to memory 2. find a disk and send msg to disk. mark sync msg to disk (direct: memory to disk, mirror oper)

            b. file/dir do exist?
                1. file/dir in memory? yes, send msg to memory. 
                2. no? 1. send msg to disk. 2. mark sync msg to memory(this is a mirror oper) (direct: disk to memory)

                3. file read? --> (no mirror oper, send to memory only)
                4. file write? ---> (mirror oper)
        '''
        print(f'handle fuse msg: {msg.event}: {msg}')
        oper_event = msg.event

        fn = msg.msg[0]

        if any(fn.ext.values()):
            # file/dir exist
            if oper_event in ('read'):
                for key in ('memory', 'ssd', 'hdd'):
                    tgt = fn.info[key][disk]
                    if tgt:
                        tgt.msg_queue.put_msg(FGWEvent(f'{tgt.disk_type.name}_{oper_event}', msg))
                        break
                else:
                    assert False, f'should never show up this line for: {oper_event}'
                    raise InvalidArgument(f'{oper_event}, invalid')
            else:

                # total msgs that gonna send
                total = sum([ len(tgt) for tgt in fn.ext.values() ])
                print(f'[FR] there are {total} msgs gonna be sent out for {fn.abs_path}')

                # build msgs 
                msgs = [ FWMsg(*msg.msg)  for a in range(total -1)]
                msgs.insert(0, msg)

                for key in ('memory', 'ssd', 'hdd'):
                    for tgt in fn.ext[key]:
                        if tgt:
                            print(f'[FR] current send msg to: {tgt}')
                            tgt.msg_queue.put_msg(FGWEvent(f'{tgt.disk_type.name}_{oper_event}', msgs.pop(0)))

        else:
            if oper_event not in ('open', 'mkdir', 'create'):
                msg.release()
                raise InvalidArgument(f'invalid request event: {oper_event}') 
            else:

                _node = self._find_memory_node()
                if _node:
                    fn.ext['memory'].append(_node)

                _node = self._find_ssd_node()
                if _node:
                    fn.ext['ssd'].append(_node)

                _node = self._find_disk_node()
                if _node:
                    fn.ext['hdd'].append(_node)

                # total msgs that gonna send
                total = sum([ len(tgt) for tgt in fn.ext.values() ])
                print(f'[FR] there are {total} msgs gonna be sent out for {fn.abs_path}')

                # build msgs 
                msgs = [ FWMsg(*msg.msg)  for a in range(total -1)]
                msgs.insert(0, msg)

                if any(fn.ext.values()):
                    for key in ('memory', 'ssd', 'hdd'):
                        for tgt in fn.ext[key]:
                            if tgt:
                                print(f'[FR] current send msg to: {tgt}')
                                tgt.msg_queue.put_msg(FGWEvent(f'{tgt.disk_type.name}_{oper_event}', msgs.pop(0)))
                else:
                    msg.release()
                    raise DiskNotAvaliable(f'There is no disk available for now {oper_event}')

    def reg_disk_evt(self):

        # hdd
        evts = [f'rsp_{DiskType.HDD.name}_{evt}' for evt in fuse_evts ]
        self.reg_events(evts, self.handle_hdd_evt)

        # memory
        evts = [f'rsp_{DiskType.MEMORY.name}_{evt}' for evt in fuse_evts ]
        self.reg_events(evts, self.handle_memory_evt)

        # ssd
        evts = [f'rsp_{DiskType.SSD.name}_{evt}' for evt in fuse_evts ]
        self.reg_events(evts, self.handle_ssd_evt)

    def handle_disk_evt(self, evt, msg):
        '''
            read from disk
        '''
        print(f'[FR]handle event {msg.event}, --> {evt}, {msg}') 

        if evt in fuse_evts:
            st, stat = msg.result
            fn = msg.msg[0]
            if st == 0:
                fn.stat = stat
            else:
                if evt in ('open', 'create', 'mkdir'):
                    assert str(fn) != '/', 'never show up this line'
                    if str(fn) in fn.parent.files:
                        # here should call parent do delete file and send msg for all disks to unlink this file.
                        # should identify action; create? or open. sometimes there's no create, just open
                        # here, we need to handle  and create FGW message. do a notify to all relative disks.
                        print(f'do delete file {fn} {fn.abs_path}')
            msg.release()

    def handle_hdd_evt(self, msg):
        return self.handle_disk_evt(msg.event.lstrip('rsp_HDD_'), msg)

    def handle_memory_evt(self, msg):
        return self.handle_disk_evt(msg.event.lstrip('rsp_MEMORY_'), msg)

    def handle_ssd_evt(self, msg):
        return self.handle_disk_evt(msg.event.lstrip('rsp_SSD_'), msg)



