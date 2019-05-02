
from .config import VMConfig

class VMConfigXml(VMConfig):
    ext = "xml"

    def serialize(self, cfg='{}'):
        print("not implementation yet...")
        pass


    def load(self):
        print("XML implementation yet...{}".format(self.path))
        pass
