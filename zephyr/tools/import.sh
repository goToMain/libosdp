#!/bin/bash

REPO_ROOT=$(git rev-parse --show-toplevel)

if [ ! -d $REPO_ROOT/zephyr ]; then
	echo "Error: this doens't look like cbsiddharth/libosdp.git"
	exit 1
fi

if [ "x$ZEPHYR_BASE" == "x" ]; then
	echo "ZEPHYR_BASE is not set. source zephyr-env.sh first"
	exit 1
fi

rm -rf $REPO_ROOT/src
cp -rf $ZEPHYR_BASE/drivers/osdp/src $REPO_ROOT/src
cp  -f $ZEPHYR_BASE/drivers/osdp/CMakeLists.txt $REPO_ROOT/zephyr
cp  -f $ZEPHYR_BASE/drivers/osdp/Kconfig $REPO_ROOT/zephyr
cp  -f $ZEPHYR_BASE/drivers/osdp/osdp_zephyr.c $REPO_ROOT/zephyr
cp  -f $ZEPHYR_BASE/include/drivers/osdp.h $REPO_ROOT/include
