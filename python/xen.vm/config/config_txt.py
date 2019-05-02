
from .config import VMConfig


class VMConfigTxt(VMConfig):
    ext = "txt"

    def serialize(self, cfg='{}'):
        print("not implementation yet...")
        pass


    def load(self):
        print("TXT not implementation yet...{}".format(self.path))
        pass
