#!/usr/bin/bash

cur_dir=$(dirname $0)
buld_dir="$cur_dir/build"

rm -rf "$buld_dir"
mkdir "$buld_dir"

cd $buld_dir
cmake .. 

cd $cur_dir
make -sj4

