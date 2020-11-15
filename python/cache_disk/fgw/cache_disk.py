
fgwctl_out  = 0xabababab
fgwctl_in   = 0xabababaa
fgwtool_in  = fgwctl_out
fgwtool_out = fgwctl_in

fuse_evts = (
    'chmod', 'chown', 'create', 'mkdir', 'open',
    'read', 'rename', 'rmdir', 'symlink', 'readlink',
    'truncate', 'unlink', 'utimens', 'write'
)
