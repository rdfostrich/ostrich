#!/bin/bash
# Run ostrich
if [ "$1" == "--debug" ]; then
    shift
    /opt/ostrich/run-debug.sh $@
else
    /opt/ostrich/build/ostrich-evaluate $@
fi
