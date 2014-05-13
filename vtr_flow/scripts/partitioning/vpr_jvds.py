#!/usr/bin/python

import argparse
import sys
import os
import shutil
import glob

sys.path.append('/usr/local/google/home/jvds/projects/vpr_net_to_hmetis')
import hypergraph_to_graph
import update_net_file
import compute_placement_compliance
import compute_hyperedge_cut

VPR_BIN="/usr/local/google/home/jvds/projects/vpr_andre/vtr_flow/../vpr/vpr"
METIS_BIN="/usr/local/google/home/jvds/Downloads/metis-5.1.0/build/Linux-x86_64/programs/gpmetis"

def main():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('--ub-factor', type=int, default=3)
    parser.add_argument('--graph-model', choices=['clique', 'star', 'none'], default='clique')
    parser.add_argument('--graph-edge-weight', choices=['1/f', '1/f2', '1'], default='1/f')
    # space out factor should be handled by vpr
    #parser.add_argument('--spaceout-factor', type=float, default=1.1)
    args, vpr_args_list = parser.parse_known_args()

    # filter out any args that were meant for us
    #vpr_args_list = filter(is_vpr_arg, sys.argv[1:])
    vpr_args = " ".join(vpr_args_list)

    # run VPR --route
    if "--route" in sys.argv:
        status = os.system("%s --net_file packed_partitioned.net %s" % (VPR_BIN, vpr_args))
        sys.exit(status)

    # remove --seed [digit] from args
    if "--seed" in vpr_args_list:
        seed_idx = vpr_args_list.index('--seed')
        # first pop removes --seed, second pop removes seed number
        vpr_args_list.pop(seed_idx)
        vpr_args_list.pop(seed_idx)
    vpr_pack_args = " ".join(vpr_args_list)

    # run VPR --pack
    status = os.system("%s --pack --net_file packed.net %s" % (VPR_BIN, vpr_pack_args))
    if status != 0:
        sys.exit(status)

    if args.graph_model != "none":
        # convert hypergraph to graph
        hypergraph_to_graph.run(blocks_file='blocks.txt', input_hypergraph='hypergraph.txt', output_graph='justgraph.txt', graph_model=args.graph_model, graph_edge_weight=args.graph_edge_weight)

        # run metis
        status = os.system("%s -ufactor=%d justgraph.txt 2" % (METIS_BIN, args.ub_factor * 10))
        if status != 0:
            sys.exit(status)
        # get the hyperedge cut
        hec = compute_hyperedge_cut.run('hypergraph.txt', 'justgraph.txt.part.2')
        with open('hyperedge_cut.txt', 'w') as f:
            f.write("%d\n" % hec)

        # get nx and ny
        nx = -1
        ny = -1
        with open('vpr_stdout.log') as f:
            for line in f:
                if "mapped into" in line:
                    parts = line.split()
                    nx = int(parts[7])
                    ny = int(parts[9])
                    break
        assert(nx != -1)
        assert(ny != -1)

        # update net file with regions
        update_net_file.run(input_net_file='packed.net', output_net_file='packed_partitioned.net', partitions_file='justgraph.txt.part.2', total_partitions=2, nx=nx, ny=ny, blocks='blocks.txt')
    else:
        shutil.copyfile('packed.net', 'packed_partitioned.net')

    status = os.system("%s --place --route --net_file packed_partitioned.net %s" % (VPR_BIN, vpr_args))
    if status != 0:
        sys.exit(status)

    # get the placement compliance
    place_file = glob.glob('*.place')[0]
    count_good, count_bad = compute_placement_compliance.run(place_file, 'packed_partitioned.net')
    with open('placement_compliance.txt', 'w') as f:
        f.write("%f\n" % (float(count_good)/float(count_good+count_bad)))

main()
