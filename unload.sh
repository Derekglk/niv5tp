#!/bin/sh

# invoke rmmod with all arguments we got
/sbin/rmmod custom_module $* || exit 1

# Remove stale nodes
rm -f /dev/chenillard





