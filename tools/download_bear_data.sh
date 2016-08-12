#!/bin/bash

location="ftp://nassdataweb.infor.uva.es/BEAR"
wget -ncr $location

mkdir -p ../data
mv nassdataweb.infor.uva.es/BEAR/* ../data
rm -rf nassdataweb.infor.uva.es

