#!/usr/bin/env bash


pushed $(pwd)
cd /root/sync

/root/sync/sync.sh syncnow /home/wwwroot/default/icbc --exclude-from=/root/sync/donotsync $@ &
popd

