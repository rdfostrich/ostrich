#!/bin/bash

location="../data"
start="1"
end="58"

i=0
for i in $(seq 1 $end); do
    echo -ne "\r$i / $end"
    iprev=$(($i - 1))
    first="$location/$iprev.nt"
    second="$location/$i.nt"
    outputlocation="$location/$i.nt.patch"
    additionslocation="$location/$i.additions.txt"
    deletionslocation="$location/$i.deletions.txt"

    # Write patch
    if [ ! -f $first ]; then
        sed "s/^/+/g" $second > $outputlocation
    else
        if [ -f $second ]; then
            diff -u <(sort $first) <(sort $second) | tr -d '\r' > $outputlocation
        fi
    fi

    # Write additions/deletions
    if [ -f $second ]; then
        grep "^+[^+]" $outputlocation | cut -c 2- > $additionslocation
        grep "^-[^-]" $outputlocation | cut -c 2- > $deletionslocation
    fi
    rm $outputlocation
    if [ -f $first ]; then
        rm $first
    fi
done
rm $second
echo ""
