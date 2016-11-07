#!/bin/bash

output=$1

# Get insertion data
if [ ! -f insertion.csv ]; then
    sed '/---INSERTION START---/,/---INSERTION END---/!d' $output | sed '1,1d; $d' > insertion.csv
fi

# Get query data
sed '/---QUERIES START/,/---QUERIES END---/!d' $output > .queries.txt
query=$(head -n 1 .queries.txt | sed 's/^.*\/\([^\/]*\)\/\([^\/]*\)---$/\1-\2/')
sed -i "" '1,1d; $d' .queries.txt

gcsplit .queries.txt '/---PATTERN START:.*/' {*} -f query-$query- > /dev/null
rm .queries.txt

mkdir $query
for file in query-$query-*; do
    if [ $(cat $file | wc -l) -gt 1 ]; then
        echo $file
        pattern=$(head -n 1 $file | sed 's/---PATTERN START: //' | sed 's/ /_/g' | sed 's/\.$//' | sed 's/\//\\/g')
        sed -i "" '1,1d; $d' $file
        targetbase="$query/"

        #sed '/--- ---VERSION MATERIALIZED$/,/--- ---DELTA MATERIALIZED/!d' $file | sed '1,1d; $d' > "$targetbase/versionmat-$pattern.csv"
        #sed '/--- ---DELTA MATERIALIZED$/,/--- ---VERSION/!d' $file | sed '1,1d; $d' > "$targetbase/deltamat-$pattern.csv"
        #sed '/--- ---VERSION$/,$!d' $file | sed '1,1d' > "$targetbase/version-$pattern.csv"

        sed '/--- ---VERSION MATERIALIZED$/,/--- ---VERSION/!d' $file | sed '1,1d; $d' > "$targetbase/versionmat-$pattern.csv"
        sed '/--- ---VERSION$/,$!d' $file | sed '1,1d' > "$targetbase/version-$pattern.csv"

        rm $file
    fi
done

#sed '/---PATTERN START/,/---PATTERN END---/!d' $output