#!/bin/sh

cur_dir=$(pwd)

rm -rf /tmp/audit_file_*.log

audit=$cur_dir/audit_out/bin/mpaudit
config=$cur_dir/audit_out/conf/audit.cfg

pushd $(pwd)
cd $cur_dir/audit_out/bin

$audit -c $config

popd
