#!/usr/bin/env python3
# -*- coding: utf-8 -*-


from dbgobserver import Observer
from dbgconfig import config, DbgDict
import enum

from  dbgactiondef import CtrlModID, CtrlEvent
from dbgfactory import BuildFactoryAutoRegister, BuildFactory
from dbgctl import dbg_ctrl_get
from dbgrule import FilterRule, ColorRule
from dbgactiondef import action_filter_type, action_target_type, dbg_print


class DbgIntf(Observer):

    _mod = None
    def __init__(self, action_ctrl = None, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.action_ctrl = action_ctrl
        self.observe(dbg_ctrl_get(self._mod.name))

        self.evt_table = {
            CtrlEvent.new:    self.evt_new,
            CtrlEvent.update: self.evt_upt,
            CtrlEvent.delete: self.evt_del
        }

    def update(self, *args, **kwargs):
        dbg_print(f"{self} >>>: {args}, {kwargs}")

        mod, evt, *rest_args = args
        assert mod is self._mod
        return self.evt_table[evt](*rest_args, **kwargs)

    def __str__(self):
        if hasattr(self, '_act') and self._act:
            return f'{self.__class__.__name__}({self._mod.name}, {self._act})'

        return f'{self.__class__.__name__}({self._mod.name})'

    def show(self):
        dbg_print(f'{self}')

    def __repr__(self):
        return self.__str__()

    def evt_del(self, *args, **kwargs):
        assert False, '{self} Should Never show up this line'

    def config_action(self, *args, **kwargs):
        enable, *_ = args

        if enable and not self._act:
            self._act = self.action_ctrl.create_action(self._mod.name)
        else:
            self._act = None

        return self.action_ctrl.write_action(self._mod, self._act)


    def evt_new(self, *args, **kwargs):

        if self._mod in (CtrlModID.Color, CtrlModID.Filter):
            assert False, '{self} Should Never show up this line'
        self._act = None

        return self.config_action(*args, **kwargs)

    def evt_upt(self, *args, **kwargs):
        self.config_action(*args, **kwargs)

        return self.action_ctrl.refresh_common_table()


class DbgIntfShowLineNumber(DbgIntf):
    _mod = CtrlModID.ShowLineNumber

class DbgIntfShowTimestamp(DbgIntf):
    _mod = CtrlModID.ShowTimeStamp

class DbgIntfShowClient(DbgIntf):
    _mod = CtrlModID.ShowClient

class DbgIntfShowServer(DbgIntf):
    _mod = CtrlModID.ShowServer

class DbgIntfShowLength(DbgIntf):
    _mod = CtrlModID.ShowLength

class DbgIntfShowEnableLog(DbgIntf):
    _mod = CtrlModID.EnableLog
    def evt_upt(self, *args, **kwargs):
        self.config_action(*args, **kwargs)

        return self.action_ctrl.refresh_post_table()

class DbgIntfShowEnableListbox(DbgIntf):
    _mod = CtrlModID.EnableListbox
    def evt_upt(self, *args, **kwargs):
        self.config_action(*args, **kwargs)

        return self.action_ctrl.refresh_post_table()

class DbgIntfColor(DbgIntf):
    _mod = CtrlModID.Color

    def evt_new(self, refresh_table = True, *args, **kwargs):

        if not hasattr(self, '_act'):
            self._act = []

        rule = ColorRule(**kwargs)
        act = None

        if rule.ignorecase:
            act = self.action_ctrl.create_action('color_ignorecase', rule)
        else:
            act = self.action_ctrl.create_action('color', rule)

        self._act.append(act)

        self.action_ctrl.write_action(self._mod, act)

        if refresh_table:
            self.action_ctrl.refresh_color_table()

    def evt_upt(self, *args, **kwargs):
        return True

    def evt_del(self, *args, **kwargs):
        return True

class DbgIntfFilter(DbgIntf):
    _mod = CtrlModID.Filter

    def evt_new(self, refresh_table = True, *args, **kwargs):
        if not hasattr(self, '_act'):
            self._act = []

        rule = FilterRule(**kwargs)
        act = self.action_ctrl.create_action(action_filter_type(rule.match_condition, rule.ignorecase), rule)

        self._act.append(act)

        self.action_ctrl.write_action(self._mod, act)

        if refresh_table:
            self.action_ctrl.refresh_filter_table()


    def evt_upt(self, *args, **kwargs):
        dbg_print(args, kwargs, "not handle for now")
        return True

    def evt_del(self, *args, **kwargs):
        return True


g_ctrl_intf = [None for x in range(len(DbgIntf.__subclasses__()) + 1)]

def dbg_intf_init(action_ctrl = None, *args, **kwargs):

    global g_ctrl_intf

    for cls in DbgIntf.__subclasses__():
        g_ctrl_intf[cls._mod.value] = cls(action_ctrl, *args, **kwargs)

def dbg_intf_show(name = None):
    global g_ctrl_intf

    if name:
        try:
            g_ctrl_intf[CtrlModID[name].value].show()
        except Exception as e:
            dbg_print(f"Unkown module: {name}, error {e}")
        finally:
            return

    for mod in g_ctrl_intf:
        if mod:
            mod.show()

if __name__ == '__main__':
    from dbgctl import dbg_ctrl_init, dbg_ctrl_notify_create
    dbg_ctrl_init()
    dbg_intf_init()
    dbg_ctrl_notify_create()

