#!/bin/bash

REPO_ROOT=$(git rev-parse --show-toplevel)

if [ ! -d $REPO_ROOT/zephyr ]; then
	echo "Error: this file has be in libosdp/zephyr. Did you move it?"
	exit 1
fi

if [ "x$ZEPHYR_BASE" == "x" ]; then
	echo "ZEPHYR_BASE is not set. source zephyr-env.sh first"
	exit 1
fi

rm -rf $ZEPHYR_BASE/drivers/osdp/
rm -f  $ZEPHYR_BASE/include/drivers/osdp.h

mkdir -p $ZEPHYR_BASE/drivers/osdp/
cp -rf $REPO_ROOT/src $ZEPHYR_BASE/drivers/osdp
cp  -f $REPO_ROOT/zephyr/CMakeLists.txt $ZEPHYR_BASE/drivers/osdp
cp  -f $REPO_ROOT/zephyr/Kconfig $ZEPHYR_BASE/drivers/osdp
cp  -f $REPO_ROOT/zephyr/osdp_zephyr.c $ZEPHYR_BASE/drivers/osdp
cp  -f $REPO_ROOT/include/osdp.h $ZEPHYR_BASE/include/drivers/osdp.h
