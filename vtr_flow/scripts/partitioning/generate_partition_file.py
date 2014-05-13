import argparse
import update_net_file
import compute_placement_compliance

def main():
    parser = argparse.ArgumentParser(description='Determines which partition each block is in, based on actual placement')

    # need to know the region boundaries
    parser.add_argument('--placement', required=True)
    parser.add_argument('--total-partitions', required=True, type=int)
    parser.add_argument('--blocks', required=True)
    parser.add_argument('--ny', required=True, type=int)
    parser.add_argument('--partition-file', required=True)

    args = parser.parse_args()

    assert(args.total_partitions == 2)

    p0_ymin, p0_ymax = update_net_file.partition_number_to_y_range(0, args.total_partitions, args.ny)
    p1_ymin, p1_ymax = update_net_file.partition_number_to_y_range(1, args.total_partitions, args.ny)

    output = open(args.partition_file, "w")
    # go through each line of blocks, look it up in placement
    placement = compute_placement_compliance.read_place_file(args.placement)
    with open(args.blocks) as f:
        for line in f:
            block_name = line.strip().split()[0]
            # actual placement of block
            pos = placement[block_name]
            # transform pos into partition index
            y = pos[1]
            if y >= p0_ymin and y <= p0_ymax:
                partition_index = 0
            else:
                partition_index = 1
            output.write("%d\n" % partition_index)

main()
