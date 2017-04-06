#!/usr/bin/env sh

#valgrind -v --time-stamp=yes --leak-check=full --log-file=/tmp/valgrind_fwd.log --track-fds=yes 
./fwd_master -s localhost -p 6165 &
