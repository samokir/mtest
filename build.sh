#!/usr/bin/bash

cur_dir=$(readlink -f $(dirname $0))
build_dir="$cur_dir/build"

rm -rf "$build_dir"
mkdir "$build_dir"

cd $build_dir
cmake ..

make -sj4
cd $cur_dir

