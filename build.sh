#!/usr/bin/env bash

name=$1
cd "$name"

check_exit () {
    if [ $? -ne 0 ]; then
        exit 1
    fi
}

if [ -f ./bootstrap ]; then
  ./bootstrap
  check_exit
fi

CC=wllvm ./configure -C -q --disable-nls CFLAGS="-O1 -g"
check_exit

CC=wllvm make -j3
check_exit

cd ..
exit 0