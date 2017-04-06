#!/usr/bin/env bash


echo 30000000 > /proc/sys/fs/inotify/max_user_watches

killall inotifywait
sleep 1

./sync.sh bemaster /home/tww/www $@ &
