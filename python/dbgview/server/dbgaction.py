#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import tkinter
from pprint import pprint

from dbgfactory import BuildFactoryAutoRegister, BuildFactory
from dbgconfig import config

from dbgactiondef import ActionType, ActionTarget, ActionFilterType, action_filter_type, action_target_type, CtrlModID, cfg_table_module_common, cfg_table_module_post, dbg_print
import threading


class Action:

    _type_act = None

    def __call__(self, *args, **kwargs):
        pass

    def __str__(self):
        return f'{self.__class__.__name__}'

    def __repr__(self):
        return self.__str__()

'''
    actions for common tables
'''
class ActionCommon(Action):
    _type_act = ActionType.common
    pass


class ActionCommonShowLineNumber(ActionCommon):
    _config = CtrlModID.ShowLineNumber.name
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        msg.prefix += '{:0>5d}   '.format(listbox.size())

class ActionCommonShowTimeStamp(ActionCommon):
    _config = CtrlModID.ShowTimeStamp.name
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        msg.prefix += f' {time.ctime(msg.tm)}'


class ActionCommonShowClient(ActionCommon):
    _config = CtrlModID.ShowClient.name
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        msg.prefix += f' {msg.client}'

class ActionCommonShowServer(ActionCommon):
    _config = CtrlModID.ShowServer.name
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        msg.prefix += f' {msg.server}'

class ActionCommonShowLength(ActionCommon):
    _config = CtrlModID.ShowLength.name
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        msg.prefix += ' {:0>5d}'.format(len(msg))

'''
string format
    "{:<10s}{:>10d}".format(word, number)
'''

'''
    actions for post config
'''
class ActionPostConfig(Action):
    _type_act = ActionType.postconfig
    pass

class ActionPostListBoxInsert(ActionPostConfig):
    _config = CtrlModID.EnableListbox.name
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        listbox.insert(tkinter.END, str(msg))

class ActionPostEnableLog(ActionPostConfig):
    _config = CtrlModID.EnableLog.name
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        print(msg)

'''
    action for colors
'''
class ActionColor(Action):
    _type_act = ActionType.color

    def __init__(self, *args, **kwargs):
        self._rule, *_ = args
        super().__init__(*args[1:], **kwargs)

    def __call__(self, *args, **kwargs):

        msg, listbox, *_ = args

        if self._rule.match(self.match, msg):
            self._rule.matched()
            listbox.itemconfig(tkinter.END, **self._rule.itemconfig)


class ActionColorMatch(ActionColor):
    _config = 'color'

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule in msg


class ActionColorMatchIgnorecase(ActionColor):
    _config = "color_ignorecase"

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule.lower() in msg.lower()


class ActionFilter(Action):
    _type_act = ActionType.filter

    def __init__(self, *args, **kwargs):
        self._rule, *_ = args

        super().__init__(*args[1:], **kwargs)

    def __call__(self, *args, **kwargs):
        if self._rule.match(self.match, *args, **kwargs):
            return self._rule.matched()
        else:
            return self._rule.not_matched()

    def match(self, *args, **kwargs):
        return ActionTarget.DROP

class ActionFilterEqual(ActionFilter):
    _config = ActionFilterType.equal.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule == msg


class ActionFilterNotEqual(ActionFilter):
    _config = ActionFilterType.not_equal.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule != msg

class ActionFilterContain(ActionFilter):
    _config = ActionFilterType.contain.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule in msg

class ActionFilterNotContain(ActionFilter):
    _config = ActionFilterType.not_contain.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule not in msg

class ActionFilterEqualIgnorecase(ActionFilter):
    _config = ActionFilterType.equal_ic.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule.lower() == msg.lower()

class ActionFilterNotEqualIgnorecase(ActionFilter):
    _config = ActionFilterType.not_equal_ic.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule.lower() != msg.lower()

class ActionFilterContainIgnorecase(ActionFilter):
    _config = ActionFilterType.contain_ic.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule.lower() in msg.lower()

class ActionFilterNotContainIgnorecase(ActionFilter):
    _config = ActionFilterType.not_contain_ic.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule.lower() not in msg.lower()

class ActionFactoryBuilder(BuildFactory):

    def __init__(self, name):
        super().__init__(name)

        self.do_register()

    def do_register(self):

        for klass in Action.__subclasses__():
            for _cls in klass.__subclasses__():

                if hasattr(_cls, '_config'):
                    self.register(_cls._config, _cls)
                else:
                    self.register(_cls.__name__, _cls)

        pprint(self.products)


class ActionManagement:
    def __init__(self):
        self.action_builder = ActionFactoryBuilder('Action')
        self.filter_mutex = threading.Lock()

        self.config_action_table = [ None for x in CtrlModID  ]
        self.config_color_action_table = [None, ]
        self.config_filter_action_table = [None, ]

        self.config_table_map = {
            CtrlModID.Color:  self.config_color_action_table,
            CtrlModID.Filter: self.config_filter_action_table,
        }


    def create_action(self, action_key, *args, **kwargs):
        return self.action_builder.create(action_key, *args, **kwargs)

    def write_action(self, mod, act, add = True):

        if mod in self.config_table_map:
            table = self.config_table_map.get(mod)

            if add and act:
                table.append(act)
            else:
                if act in table:
                    table.remove(act)
        else:
            self.config_action_table[mod.value] = act

        return True

    def refresh_common_table(self):
        self.action_table_common = []

        for mod in cfg_table_module_common:
            act = self.config_action_table[mod.value]

            if act:
                self.action_table_common.append(act)

        dbg_print(f"common_table: {self.action_table_common}")

    def refresh_post_table(self):
        self.action_table_post = []

        for mod in cfg_table_module_post:
            act = self.config_action_table[mod.value]

            if act:
                self.action_table_post.append(act)

        dbg_print(f"action_table: {self.action_table_post}")


    def refresh_color_table(self):
        self.action_table_color = []

        for act in self.config_color_action_table:

            if act:
                self.action_table_color.append(act)

        dbg_print(f"color action table: {self.action_table_color}")


    def refresh_filter_table(self):

        with self.filter_mutex:
            self.action_table_filter = []

            for act in self.config_filter_action_table:
                if act:
                    self.action_table_filter.append(act)

        dbg_print(f"filter action table: {self.action_table_filter}")

    def refresh_table(self):
        self.refresh_common_table()
        self.refresh_post_table()
        self.refresh_color_table()
        self.refresh_filter_table()

    def gui_action(self, msg, listbox, **kwargs):

        for act in self.action_table_common + \
                   self.action_table_post + \
                   self.action_table_color:
            act(msg, listbox, **kwargs)


    def filter_action(self, msg, *args, **kwargs):

        ret = ActionTarget.DROP

        with self.filter_mutex:
            for act in self.action_table_filter:

                tgt = act(msg, *args, **kwargs)
                if tgt is not ActionTarget.CONTINUE:
                    ret = tgt
                    break
        return ret
                
if __name__ == '__main__':
    am = ActionManagement()
