#!/usr/bin/bash

cur_dir=$(readlink -f $(dirname $0))
build_dir="$cur_dir/build"

sudo tuna --cpus="$1,$2" --isolate --irqs='*' --spread

$build_dir/mtest "$@"
