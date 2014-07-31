#!/usr/bin/env python

import argparse
import sys
import os
import shutil
import glob

sys.path.append(os.path.join(os.path.dirname(__file__), "../vtr_flow/scripts/partitioning"))

import hypergraph_to_graph
import update_net_file
import compute_placement_compliance
import compute_hyperedge_cut
import generate_logical_block_placement_constraints

VPR_BIN=os.path.join(os.path.dirname(__file__), "vpr")
METIS_BIN=os.path.join(os.path.dirname(__file__), "gpmetis")
ROUTE_LOG_FILE='vpr_jvds.route.out'
PACK_LOG_FILE='vpr_jvds.pack.out'

LOGICAL_HYPERGRAPH_FILE='logical_hypergraph.txt'
LOGICAL_BLOCKS_FILE='logical_blocks.txt'

CLB_HYPERGRAPH_FILE='clb_hypergraph.txt'
CLB_BLOCKS_FILE='clb_blocks.txt'

PLACEMENT_CONSTRAINTS_ONLY=False
PASS_CONSTRAINTS_TO_PLACER=True

def read_stage():
    stage = 0
    try:
        with open("stage.txt") as f:
            for line in f:
                line = line.strip()
                if len(line) > 0:
                    stage = int(line)
    except IOError:
        pass
    return stage

def write_stage(stage):
    with open("stage.txt", "w") as f:
        f.write("%d\n" % stage)

def main():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('--staged', dest='staged', action='store_true', default=False)
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
        vpr_cmd = "%s --net_file packed_partitioned.net %s" % (VPR_BIN, vpr_args)
        log = open(ROUTE_LOG_FILE, 'a')
        log.write(vpr_cmd + "\n")
        log.close()
        status = os.system(vpr_cmd)
        sys.exit(status)

    # remove --seed [digit] from args
    if "--seed" in vpr_args_list:
        seed_idx = vpr_args_list.index('--seed')
        # first pop removes --seed, second pop removes seed number
        vpr_args_list.pop(seed_idx)
        vpr_args_list.pop(seed_idx)
    vpr_pack_args = " ".join(vpr_args_list)

    if args.staged:
        stage = read_stage()
        print "read stage %s" % stage
    else:
        stage = -1

    # run VPR --pack
    vpr_cmd = "%s --pack --net_file packed.net %s" % (VPR_BIN, vpr_pack_args)
    if stage in [-1, 0]:
        log = open(PACK_LOG_FILE, 'a')
        log.write(vpr_cmd + "\n")
        log.close()
        status = os.system(vpr_cmd)
        if status != 0:
            sys.exit(status)

    if stage in [-1, 1]:
        if args.graph_model != "none":
            if PLACEMENT_CONSTRAINTS_ONLY:
                input_hypergraph = CLB_HYPERGRAPH_FILE
                input_blocks = CLB_BLOCKS_FILE
            else:
                input_hypergraph = LOGICAL_HYPERGRAPH_FILE
                input_blocks = LOGICAL_BLOCKS_FILE
            # convert hypergraph to graph
            hypergraph_to_graph.run(blocks_file=input_blocks, input_hypergraph=input_hypergraph, output_graph='justgraph.txt', graph_model=args.graph_model, graph_edge_weight=args.graph_edge_weight)

            # run metis
            status = os.system("%s -ufactor=%d justgraph.txt 2" % (METIS_BIN, args.ub_factor * 10))
            if status != 0:
                sys.exit(status)

            # get the hyperedge cut
            hec = compute_hyperedge_cut.run(input_hypergraph, 'justgraph.txt.part.2')
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

            if PLACEMENT_CONSTRAINTS_ONLY:
                # update net file with regions
                update_net_file.run(input_net_file='packed.net', output_net_file='packed_partitioned.net', partitions_file='justgraph.txt.part.2', total_partitions=2, nx=nx, ny=ny, blocks=input_blocks)
            else:
                generate_logical_block_placement_constraints.run(output_partitions='placement_regions.txt', partitions_file='justgraph.txt.part.2', total_partitions=2, nx=nx,ny=nx)
                # run the damn thing again, hoping it uses the new placement regions file
                status = os.system(vpr_cmd)
                if PASS_CONSTRAINTS_TO_PLACER:
                    shutil.copyfile('packed.net', 'packed_partitioned.net')
                else:
                    update_net_file.remove_regions('packed.net', 'packed_partitioned.net')
        else:
            shutil.copyfile('packed.net', 'packed_partitioned.net')

    if stage in [-1, 2]:
        status = os.system("%s --place --route --net_file packed_partitioned.net %s" % (VPR_BIN, vpr_args))
        if status != 0:
            sys.exit(status)

        # get the placement compliance
        place_file = glob.glob('*.place')[0]
        count_good, count_bad = compute_placement_compliance.run(place_file, 'packed_partitioned.net')
        with open('placement_compliance.txt', 'w') as f:
            f.write("%f\n" % (float(count_good)/float(count_good+count_bad)))

    if args.staged:
        # set stage for next run
        write_stage(stage + 1)

main()
