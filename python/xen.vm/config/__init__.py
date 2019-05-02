#!/usr/bin/env python
# *-* coding: utf-8 -*-

from .config_json import VMConfigJson
from .config_xml import VMConfigXml
from .config_txt import VMConfigTxt


class ConfigFileMethodNotFound(Exception): pass

class VMConfigProvider:
    def __init__(self):
        self._ins = {}

    def reg(self, key, val):
        if key not in self._ins:
            self._ins[key] = val

    def get(self, key):
        try:
            return self._ins[key]
        except KeyError:
            raise ConfigFileMethodNotFound(f"config file for extension .{key} not found")


_vms = VMConfigProvider()
_vms.reg(VMConfigJson.ext, VMConfigJson)
_vms.reg(VMConfigXml.ext,  VMConfigXml)
_vms.reg(VMConfigTxt.ext,  VMConfigTxt)


__all__ = ('load_config', 'write_config')


def _get_configer(fl_path):

    ext = fl_path.split('.')[-1]

    return _vms.get(ext)


def load_config(fl_path):

    CFG = None
    try:
        CFG = _get_configer(fl_path)
    except ConfigFileMethodNotFound as e:
        print("Warrning: " + str(e))

    finally:
        return CFG(fl_path).load() if CFG else "{}"


def write_config(cfg, fl_path):

    CFG = None
    try:
        CFG = _get_configer(fl_path)

    except ConfigFileMethodNotFound as e:
        print("Warrning: " + str(e))

    finally:
        return CFG(fl_path).serialize(cfg) if CFG else False
