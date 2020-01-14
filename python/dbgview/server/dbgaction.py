#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import enum
from dbgfactory import BuildFactoryAutoRegister, BuildFactory
from dbgconfig import config, DbgDict
import time
import tkinter
from pprint import pprint

ActionType = enum.Enum('ActionType', "preconfig common postconfig filter color")
ActionTarget = enum.Enum('ActionTarget', 'DROP ACCEPT CONTINUE RETURN')
ActionFilterType = enum.Enum('ActionFilterType', 'equal not_equal contain not_contain')

class Action:

    _type_act = None

    def __call__(self, *args, **kwargs):
        pass

    def __str__(self):
        return f'{self.__class__.__name__}'

    def __repr__(self):
        return self.__str__()

'''
    actions for Preconfig
'''

class ActionPreConfig(Action):
    _type_act = ActionType.preconfig
    pass


'''
    actions for common tables
'''
class ActionCommon(Action):
    _type_act = ActionType.common
    pass


class ActionCommonShowLineNumber(ActionCommon):
    _config = "show_line_number"
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        msg.prefix += '{:<12d}'.format(listbox.size())

class ActionCommonShowTimeStamp(ActionCommon):
    _config = "show_timestamp"
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        msg.prefix += f' {time.ctime(msg.tm)}'

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
    _config = "set_listbox"
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        listbox.insert(tkinter.END, str(msg))

'''
    action for colors
'''
class ActionColor(Action):
    _type_act = ActionType.color
    pass

class ActionFilter(Action):
    _type_act = ActionType.filter
    pass


class ActionFilterEqual(ActionFilter):
    _config = ActionFilterType.equal.name
    pass

class ActionFilterNotEqual(ActionFilter):
    _config = ActionFilterType.not_equal.name
    pass

class ActionFilterContain(ActionFilter):
    _config = ActionFilterType.contain.name
    pass

class ActionFilterNotContain(ActionFilter):
    _config = ActionFilterType.not_contain.name
    pass

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
        self._build_action_table()

    def _build_action_table(self):

        self.build_action_table_pre()
        self.build_action_table_common()
        self.build_action_table_post()
        self.build_action_table_filter()
        self.build_action_table_color()

    def build_action_table_pre(self):
        self.action_table_pre = []

        for key in config.preconfig:
            pass

    def build_action_table_common(self):

        self.action_table_common = []

        for key in config.common:
            if config.common.get(key, False):
                self.action_table_common.append(self.action_builder.create(key))

        print("common_table: ", self.action_table_common)

    def build_action_table_post(self):
        self.action_table_post = []

        for key in config.postconfig:
            if config.postconfig.get(key, False):
                print(key)
                self.action_table_post.append(self.action_builder.create(key))

        print("action_table: ", self.action_table_post)


    def build_action_table_color(self):
        self.action_table_color = []

        for key in config.color:
            print(key)

    def build_action_table_filter(self):
        self.action_table_filter = []

        for key in config.filter:
            print(key)

    def gui_action(self, msg, listbox, **kwargs):

        for act in self.action_table_pre + \
                   self.action_table_common + \
                   self.action_table_post + \
                   self.action_table_color:
            act(msg, listbox, **kwargs)


    def filter_action(self, msg, *args, **kwargs):
        
        for act in self.action_table_filter:

            tgt = act(msg, *args, **kwargs)
            if tgt in (ActionTarget.DROP, ActionTarget.ACCEPT, ActionTarget.RETURN):
                return tgt
                
if __name__ == '__main__':
    am = ActionManagement()
