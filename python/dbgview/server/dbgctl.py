
from dbgobserver import Observable
from dbgconfig import config, DbgDict
import enum


CtrlModID = enum.Enum('CtrlModID', 'ShowLineNumber ShowTimeStamp ShowClient ShowServer ShowLength EnableLog')
CtrlID = enum.Enum('CtrlID', 'new update delete')


class DbgCtrl(Observable):

    _inst = None
    _mod  = 

    def __new__(cls, *args, **kwargs):
        if not cls._inst:
            cls._inst = super().__new__(*args, **kwargs)

        return cls._inst


    def __init__(self, *args, **kwargs):
        self.notify_create()

    def notify_update(self):
        pass

    def notify_create(self):
        pass

    def notify_delete(self):
        pass


class DbgCtrlShowLineNumber(DbgCtrl):
    pass

class DbgCtrlShowTimestamp(DbgCtrl):
    pass


class DbgCtrlShowClient(DbgCtrl):
    pass


class DbgCtrlShowServer(DbgCtrl):
    pass


class DbgCtrlShowLength(DbgCtrl):
    pass


class DbgCtrlEnableLog(DbgCtrl):
    pass


class CtrlEnableListBox(DbgCtrl):
    pass


class DbgCtrlEnableColor(DbgCtrl):
    pass

class DbgCtrlEnableFilter(DbgCtrl):
    pass


class DbgCtrlColor(DbgCtrl):
    pass


class DbgCtrlFilter(DbgCtrl):
    pass

