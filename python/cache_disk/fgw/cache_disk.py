import enum

fgwctl_out  = 0xabababab
fgwctl_in   = 0xabababaa
fgwtool_in  = fgwctl_out
fgwtool_out = fgwctl_in

fuse_evts = (
    'chmod', 'chown', 'create', 'mkdir', 'open',
    'read', 'rename', 'rmdir', 'symlink', 'readlink',
    'truncate', 'unlink', 'utimens', 'write', 'release'
)


DiskType = enum.Enum(
    value = 'DiskType',
    names = 'HDD SSD MEMORY'
)

DiskStatus = enum.Enum(
    value = 'DiskStatus',
    names = 'INIT SCANNING RUNING STOPPED ERROR'
)

FileStatus = enum.Enum(
    value = 'FileStatus',
    names = 'closed opened sync'
)
