
[toc]

## install system

### create a disk
```
qemu-img create img.name.img 100G
```

### install system
```
    qemu-system-x86_64 -m 1G -boot d -drive file=/home/tww/Downloads/SLE-15-SP3-Full-x86_64-GM-Media1.iso,media=cdrom -cpu host --enable-kvm -monitor stdio -drive file=suse15sp3.img,format=raw
```


### boot system 
```
    qemu-system-x86_64 -m 1G -boot d -cpu host --enable-kvm -monitor stdio -drive file=suse15sp3.img,format=raw
```



