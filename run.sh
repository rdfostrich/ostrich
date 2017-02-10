#!/bin/bash
# Run ostrich
if [ "$1" == "--debug" ]; then
    shift
    /opt/patchstore/run-debug.sh $@
else
    /opt/patchstore/build/tpfpatch_store $@
fi
