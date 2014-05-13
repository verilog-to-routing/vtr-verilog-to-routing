#!/bin/bash
set -e
VPR_BIN=/usr/local/google/home/jvds/projects/vpr_andre/vtr_flow/../vpr/vpr
if [[ "$@" == *--route* ]]
then
  ${VPR_BIN} --net_file packed_partitioned.net $@
else
  PACK_ARGS=`echo $@ | sed -e 's/--seed [0-9]*//'`
  ${VPR_BIN} --pack --net_file packed.net ${PACK_ARGS}
  # run hmetis
  /usr/local/google/home/jvds/Downloads/hmetis-1.5-linux/shmetis hypergraph.txt 2 1
  # get nx and ny
  NX=`grep 'mapped into' vpr_stdout.log | cut -d ' ' -f 8`
  NY=`grep 'mapped into' vpr_stdout.log | cut -d ' ' -f 10`
  # update net file with regions
  python ~/projects/vpr_net_to_hmetis/update_net_file.py --input-net-file packed.net --output-net-file packed_partitioned.net --partitions hypergraph.txt.part.2 --total-partitions 2 --nx ${NX} --ny ${NY} --blocks blocks.txt
  ${VPR_BIN} --place --route --net_file packed_partitioned.net $@
fi
