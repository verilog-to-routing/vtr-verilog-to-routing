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
export CORES=$(nproc --all)
echo "Cores: $CORES"
echo
echo "Memory"
echo "----------------------------------------"
cat /proc/meminfo
echo "----------------------------------------"
export MEM_GB=$(($(awk '/MemTotal/ {print $2}' /proc/meminfo)/(1024*1024)))
echo "Memory (GB): $CORES"
export MEM_CORES=$(($MEM_GB/4))
echo "Cores (4 GB per): $MEM_CORES"
export MAX_CORES_NO_MIN=$(($MEM_CORES>$CORES?$CORES:$MEM_CORES))
export MAX_CORES=$(($MAX_CORES_NO_MIN<1?1:$MAX_CORES_NO_MIN))
echo "Max cores: $MAX_CORES"
echo "----------------------------------------"
echo "Process limits:"
echo "----------------------------------------"
ulimit -a
echo "----------------------------------------"
