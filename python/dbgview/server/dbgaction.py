#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import enum

ActionType = enum.Enum('ActionType', "filter color")



class Action:

    _inst_act = None
    _type_act = None

    def __new__(cls, *args, **kwargs):

        if not cls._inst_act:
            cls._inst_act = super().__new__(cls, *args, **kwargs)

        return cls._inst_act




class ActionFilter(Action):
    pass
