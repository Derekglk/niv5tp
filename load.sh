#!/bin/sh

/sbin/insmod ./custom_module.ko $* || exit 1

chenillard_major=`cat /proc/devices | awk "\\$2==\"chenillard\" {print \\$1}"`


rm -f /dev/chenillard
mknod /dev/chenillard c $chenillard_major 129








