#!/bin/bash

set -e

echo
echo "========================================"
echo "Host Environment"
echo "----------------------------------------"
export
echo "----------------------------------------"

echo
echo "========================================"
echo "Host CPU"
echo "----------------------------------------"

export LOGICAL_CORES=$(nproc --all)
echo "Logical Cores (counting SMT/HT): $LOGICAL_CORES"

#The 2nd column of lscpu -p contains the core ID, with each physical
# core having 1 or (in the case of SMT/hyperthreading) more logical
# CPUs. Counting the *unique* core IDs across lines tells us the number
# of *physical* CPUs (cores).
export PHYSICAL_CORES=$(lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l)
echo "Physical Cores (ignoring SMT/HT): $PHYSICAL_CORES"

export MEM_GB=$(($(awk '/MemTotal/ {print $2}' /proc/meminfo)/(1024*1024)))
echo "Memory (GB): $MEM_GB"

echo
echo "CPU Details"
echo "----------------------------------------"
lscpu
echo "----------------------------------------"
echo
echo "Memory"
echo "----------------------------------------"
cat /proc/meminfo
echo "----------------------------------------"
echo "----------------------------------------"
echo "Process limits:"
echo "----------------------------------------"
ulimit -a
echo "----------------------------------------"
