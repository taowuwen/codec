
from dbgobserver import Observable
from dbgconfig import config, DbgDict
from dbgactiondef import CtrlModID, CtrlEvent
from dbgfactory import BuildFactoryAutoRegister, BuildFactory
from dbgactiondef import  cfg_table_module_common, cfg_table_module_post, dbg_print


class DbgCtrl(Observable):

    _inst = None
    _mod  = None

    def __new__(cls, *args, **kwargs):
        if not cls._inst:
            cls._inst = super().__new__(cls, *args, **kwargs)

        return cls._inst

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)


    def notify_evt(self, evt):
        if self._mod in cfg_table_module_common:
            return self.notify(self._mod, evt, config.common.get(self._mod.name, False))

        if self._mod in cfg_table_module_post:
            return self.notify(self._mod, evt, config.postconfig.get(self._mod.name, False))

        assert False, "Never show up this line"

    def notify_update(self):
        return self.notify_evt(CtrlEvent.update)

    def notify_create(self):
        assert self._mod in (cfg_table_module_post + cfg_table_module_common)
        return self.notify_evt(CtrlEvent.new)

    def notify_delete(self):
        assert self._mod not in (cfg_table_module_post + cfg_table_module_common), "Never show up this line"
        pass

    @property
    def enable(self):
        if self._mod in cfg_table_module_common:
            return config.common.get(self._mod.name, False)

        if self._mod in cfg_table_module_post:
            return config.postconfig.get(self._mod.name, False)

        assert False, "Never Show up this line"

    @enable.setter
    def enable(self, enable):
        dbg_print(f"notice {self} --- UPDATE ---> Show?: {enable}")
        if self._mod in cfg_table_module_common:
            config.common[self._mod.name] = enable
            return self.notify_update()

        if self._mod in cfg_table_module_post:
            config.postconfig[self._mod.name] = enable
            return self.notify_update()

        assert False, "Never show up this line"

    def __str__(self):
        return self.__class__.__name__

    def show(self):
        dbg_print(f'{self} enable? {"yes" if self.enable else "no"}')

class DbgCtrlShowLineNumber(DbgCtrl):
    _mod = CtrlModID.ShowLineNumber

class DbgCtrlShowTimestamp(DbgCtrl):
    _mod = CtrlModID.ShowTimeStamp


class DbgCtrlShowClient(DbgCtrl):
    _mod = CtrlModID.ShowClient
    pass


class DbgCtrlShowServer(DbgCtrl):
    _mod = CtrlModID.ShowServer
    pass


class DbgCtrlShowLength(DbgCtrl):
    _mod = CtrlModID.ShowLength
    pass


class DbgCtrlEnableLog(DbgCtrl):
    _mod = CtrlModID.EnableLog
    pass


class DbgCtrlEnableListbox(DbgCtrl):
    _mod = CtrlModID.EnableListbox
    pass


class DbgCtrlColor(DbgCtrl):
    _mod = CtrlModID.Color

    def notify_update(self):
        return self.notify_evt(CtrlEvent.update)

    def notify_create(self):
        for key in config.Color:
            if not self.notify(self._mod, CtrlEvent.new, **key):
                return False

        return True

    def notify_delete(self):
        pass

    def show(self):
        dbg_print(f"{self}")
        for key in config.Color:
            dbg_print(f'{key}')

    def add(self, *args, **kwargs):
        pass

    def upt(self, *args, **kwargs):
        pass

    def delete(self, *args, **kwargs):
        pass

class DbgCtrlFilter(DbgCtrl):
    _mod = CtrlModID.Filter

    def notify_update(self):
        return self.notify_evt(CtrlEvent.update)

    def notify_create(self):
        for key in config.Filter:
            if not self.notify(self._mod, CtrlEvent.new, **key):
                return False

        return True

    def notify_delete(self):
        pass

    def add(self, *args, **kwargs):
        pass

    def delete(self, *args):
        pass

    def upt(self, *args, **kwargs):
        pass

    def show(self):
        dbg_print(f"{self}")
        for key in config.Filter:
            dbg_print(f'{key}')


class DbgCtrlFactory(BuildFactoryAutoRegister):
    def do_register(self, klasses):

        for klass in klasses:
            for _cls in klass.__subclasses__():
                self.register(_cls._mod.name, _cls)

g_dbg_ctrl_factory = None
g_dbg_ctrl = [None for x in range(len(DbgCtrl.__subclasses__()) + 1)]

def dbg_ctrl_init():
    global g_dbg_ctrl_factory
    g_dbg_ctrl_factory = DbgCtrlFactory('DbgCtrlFactory', [DbgCtrl])

def dbg_ctrl_get(s_ctrl = None, *args, **kwargs):

    global g_dbg_ctrl_factory
    global g_dbg_ctrl

    ctrl = g_dbg_ctrl_factory.create(s_ctrl, *args, **kwargs)
    g_dbg_ctrl[ctrl._mod.value] = ctrl

    return ctrl


def dbg_ctrl_notify_create():
    global g_dbg_ctrl

    for ctrl in g_dbg_ctrl:
        if not ctrl:
            continue

        if not ctrl.notify_create():
            return False

    return True

def dbg_controller(mod):
    global g_dbg_ctrl
    return g_dbg_ctrl[mod.value]

def dbg_ctrl_show(name = None):

    global g_dbg_ctrl

    if name:
        try:
            g_dbg_ctrl[CtrlModID[name].value].show()
        except Exception as e:
            dbg_print(f"Unkown module: {name}, error {e}")
        finally:
            return

    for mod in g_dbg_ctrl:
        if mod:
            mod.show()

if __name__ == '__main__':

    dbg_ctrl_init()

    for k in CtrlModID:
        ctrl = dbg_ctrl_get(k.name)
        print("first create", id(ctrl), ctrl, k.name, k.value)

    dbg_ctrl_notify_create()
