#!/bin/bash

TIME_VEC_WIDTHS=(1 2 4 8 16 32)
#TIME_VEC_WIDTHS=(1)

CONFIGS=("NO_SIMD" "AUTO_VEC" "INTRINSIC")

for CONFIG in ${CONFIGS[@]}
do
    for VEC_WIDTH in ${TIME_VEC_WIDTHS[@]}
    do
        make clean

        if [ "$CONFIG" == "NO_SIMD" ]; then
            make -j8 EXTRA_FLAGS="-DTIME_VEC_WIDTH=$VEC_WIDTH -fno-tree-vectorize"
        elif [ "$CONFIG" == "AUTO_VEC" ]; then
            make -j8 EXTRA_FLAGS="-DTIME_VEC_WIDTH=$VEC_WIDTH"
        elif [ "$CONFIG" == "INTRINSIC" ]; then
            make -j8 EXTRA_FLAGS="-DTIME_VEC_WIDTH=$VEC_WIDTH -DSIMD_INTRINSICS"
        else
            echo "ERROR: INVALID CONFIG $CONFIG"
            exit
        fi

        echo "CONFIG: $CONFIG, VEC_WIDTH: $VEC_WIDTH"
        ./sta /project/work/timing_analysis/openCV/pre_packing_timing_graph.echo | grep -P "Vec Width|alignof|AVG:"
    done
done
