
from .config import VMConfig

import json

class VMConfigJson(VMConfig):
    ext = "json"

    def serialize(self, cfg='{}'):
        try:
            self.write(json.dumps(cfg, indent=4))

        except PermissionError:
            print("Warning, File {self.path} has no write permission")
            return False

        return True

    def load(self):
        cfg = {}

        try:
            cfg = json.loads(self.read())
        except FileNotFoundError:
            print("Warning, file {self.path} not exist".format(self=self))
        except PermissionError:
            print("Warning, File {self.path} has no read permission".format(self=self))

        finally:
            return cfg
