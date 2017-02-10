#!/bin/bash
# Debug OSTRICH using coredumps
# Based on: https://github.com/dreamcat4/docker-images/blob/master/tvh/README.md#debugging-tvheadend

_cleanup () {
	if [ "$core_pattern_orig" ]; then
		# Restore core path
		echo "$core_pattern_orig" > /proc/sys/kernel/core_pattern
	fi
	exit 0
}
trap _cleanup TERM INT QUIT HUP

# Remember the folder ownership for later
crash_uid="$(stat -c %u /crash)"
crash_gid="$(stat -c %g /crash)"
crash_rwx="$(stat -c %a /crash)"

# Enable core dumps
ulimit -c unlimited
echo "Set: ulimit -c unlimited"

# Override core path
mkdir /crash_tmp
core_pattern_orig=$(cat /proc/sys/kernel/core_pattern)
core_pattern="/crash_tmp/%e-ostrich.t%t.core.new"
echo "$core_pattern" > /proc/sys/kernel/core_pattern

# Make sure our core files get saved as '/crash/core*'
if [ "$?" -ne "0" ]; then
	echo "error: can't modify /proc/sys/kernel/core_pattern
Did you run this image with --privileged=true flag?"
fi

if [ "$(cat /proc/sys/kernel/core_pattern | grep -v -e '^/crash_tmp/.*.core.new')" ]; then
	echo "error: the save path of core files is not /crash_tmp/.*.core.new
Aborting."
	exit 1
fi
echo "Set: core_pattern=$core_pattern"

# Run ostrich
/opt/patchstore/build/tpfpatch_store $@

# Restore core path
echo "$core_pattern_orig" > /proc/sys/kernel/core_pattern

# Make log file
uname -a > "/crash/ostrich.log"
echo /opt/patchstore/build/tpfpatch_store "$@"   >> "/crash/ostrich.log"

# Exit cleanly if there was no segfault crash
cp /crash_tmp/* /crash
core_file_new="$(find /crash -name '*core.new*' | tail -n 1)"
if [ ! "$core_file_new" ]; then
	# Restore original file ownership and permissions
	chown -R ${crash_uid}:${crash_gid} /crash && chmod ${crash_rwx} /crash
	exit 0
fi

echo "Discovered a crash!"

# Rename files so they don't conflict with other sessions
file_prefix="${core_file_new%.core.new.*}"
core_file="${file_prefix}.core"
mv "$core_file_new" "$core_file"

echo "
***********************************************************************
GDB Backtrace
***********************************************************************
"
echo "set logging on ${file_prefix}.gdb.txt
set pagination off
bt full" | gdb /opt/patchstore/build/tpfpatch_store "$core_file"
echo "
***********************************************************************
"

# Restore original file ownership and permissions
chown -R ${crash_uid}:${crash_gid} /crash && chmod ${crash_rwx} /crash

exit 1