
import json
import os
import sys

from dbgactiondef import action_target_type, ActionTarget

class DbgDict(dict):

    def __getattr__(self, key):
        return self.__getitem__(key)

    def __setattr__(self, key, val):
        return self.__setitem__(key, val)


class DbgConfig:

    def __init__(self, *args, **kwargs):

        self._fl = None
        if args:
            self._fl, *_ = args
            self._fl = os.path.expandvars(os.path.expanduser(self._fl))

        self.kwargs = DbgDict()

        if not self._fl or not os.path.exists(self._fl):
            self.set_default_config()

            if self._fl and not os.path.exists(os.path.dirname(self._fl)):
                os.mkdir(os.path.dirname(self._fl))

            self.searilize()
        else:
            self.load_cfg()

        if kwargs:
            self.kwargs.update(kwargs)

    def update(self, **kwargs):
        self.kwargs.update(kwargs)

    def load_cfg(self):

        with open(self._fl, 'r') as f:

            def parse_dict(obj):
                if isinstance(obj, dict):
                    act = obj.get('default_action_target', None)
                    if act:
                        obj['default_action_target'] = action_target_type(act)

                    return DbgDict(obj)
                return obj

            self.kwargs = json.loads(f.read(), object_hook = parse_dict)

    def __getattr__(self, key):
        try:
            return super().__getattribute__(key)
        except AttributeError:
            return self.__getitem__(key)

    def __setattr__(self, key, val):
        try:
            object.__setattr__(self, key, val)
        except AttributeError:
            self.__setitem__(key, val)

    def __getitem__(self, item):
        return self.kwargs.get(item, None)

    def __setitem__(self, key, val):
        if key and val:
            self.kwargs[key] = val
        
    def set_default_config(self):
        self.kwargs['gui'] = DbgDict({
            'default_action_target': 'accept',
            })

        self.kwargs['gui_font'] = DbgDict({
            "fg":               "black",
            "bg":               "white",
            "font":             "systemSystemFont",
            "height":           10,
            "width":            10,
            "selectforeground": "white",
            "selectbackground": "#33B397"
        })

        self.kwargs['common'] = DbgDict({
            "ShowLineNumber":    True,
            "ShowTimeStamp":      True,
            "ShowClient":         False,
            "ShowServer":         False,
            "ShowLength": False,
        })

        self.kwargs['postconfig'] = DbgDict({
            "EnableListbox": True,
            "EnableLog":  False,
        })

        self.kwargs['Color'] = [ 
            DbgDict({
                "enable":          True,
                "name":            "Error",
                "rule":            'error',
                "fg":              '#ff0000',
                "match_condition": "contain",
                "ignorecase":      True
            }),
            DbgDict({
                "enable":          True,
                "name":            "Info",
                "rule":            'Info',
                "fg":              '#00aa00',
                "match_condition": "contain",
                "ignorecase":      True
            }),

            DbgDict({
                "enable":          True,
                "name":            "Warn",
                "rule":            'Warn',
                "fg":              '#aa0000',
                "match_condition": "contain",
                "ignorecase":      True
            }),

            DbgDict({
                "enable":          True,
                "name":            "notice",
                "rule":            'notice',
                "fg":              'blue',
                "match_condition": "contain",
                "ignorecase":      True
            }),
        ]
        self.kwargs['Filter'] = [
            DbgDict({
                "enable":          True,
                "name":            "BLUE",
                "rule":            'BLUE',
                "target":          'drop',
                "match_condition": "contain",
                "ignorecase":      False
            }),
            DbgDict({
                "enable":          True,
                "name":            "filter",
                "rule":            '',
                "target":          'accept',
                "match_condition": "contain",
                "ignorecase":      False
            }),
        ]


    def searilize(self):

        def json_encode(obj):
            if isinstance(obj, ActionTarget):
                return obj.name.lower()

        with open(self._fl, 'w') as fp:
            json.dump(self.kwargs, fp, indent=4, default=json_encode)

    def __str__(self):
        return f'{self.kwargs}'

    def __repr__(self):
        return self.__str__()

config = DbgConfig('~/.config/dbgview/dbgview.json')

if __name__ == '__main__':
    from pprint import pprint
    pprint(config)
    pprint(config.common)
    pprint(config.postconfig)
    pprint(config.filter)
    pprint(config.color)
    pprint(config.gui_font)

    pprint(config.common.ShowLineNumber)

    config.common.ShowLineNumber = True
    pprint(config.common.ShowLineNumber)

    config.common.tmpkey = "tmpval"
    pprint(config.common.tmpkey)

    key = 'tmpkey'

    print(config.common.get(key))

    print(config.gui.default_action_target)
