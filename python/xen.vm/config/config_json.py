
from .config import VMConfig

import json

class VMConfigJson(VMConfig):
    ext = "json"

    def serialize(self, cfg='{}'):
        try:
            self.write(json.dumps(cfg))

        except PermissionError:
            print("Warning, File {self.path} has no write permission")
            return False

        return True

    def load(self):
        cfg = {}

        try:
            cfg = json.loads(self.read())
        except FileNotFoundError:
            print(f"Warning, file {self.path} not exist")
        except PermissionError:
            print("Warning, File {self.path} has no read permission")

        finally:
            return cfg
