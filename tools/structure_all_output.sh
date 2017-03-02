#!/bin/bash

dir=$1
for type in $(ls $dir); do
    for file in $(ls $dir/$type); do
       ./structure_eval_output.sh $dir/$type/$file
    done
done
