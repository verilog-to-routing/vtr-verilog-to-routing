import sys
import argparse
import xml.etree.ElementTree as ET

# Output: partitions[block index] = partition number
def read_partitions(filepath):
    partitions = []
    # line number = block index
    with open(filepath) as f:
        for line in f:
            partitions.append(int(line.strip()))

    return partitions

# Output: blocks[block name] = block index
def read_blocks(filepath):
    blocks = {}
    line_num = 0
    with open(filepath) as f:
        for line in f:
            # line has block name, block type
            block_name = line.strip().split()[0]
            assert(block_name not in blocks)
            blocks[block_name] = line_num
            line_num += 1

    return blocks

# 0-indexed partition number -> (y1, y2)
def partition_number_to_y_range(partition_number, total_partitions, ny):
    partition_size = (ny+2)/total_partitions
    bottom_edge = partition_size * partition_number
    if partition_number == total_partitions - 1:
        top_edge = ny+1
    else:
        top_edge = partition_size * (partition_number+1) - 1

    return (bottom_edge, top_edge)

def parse_signal_index(signal):
    open_bracket = signal.index('[')
    close_bracket = signal.index(']')
    index = int(signal[open_bracket+1:close_bracket])

    return index

def annotate_block_with_regions(blocks, block, partitions, total_partitions, nx, ny):
    # get the partition for this block
    block_index = blocks[block.get('name')]
    p = partitions[block_index]
    #print "%s %s %s" % (block.get('name'), block_index, p)

    x1 = 0
    x2 = nx+1
    y1, y2 = partition_number_to_y_range(p, total_partitions, ny)

    regions = ET.SubElement(block, 'regions')
    regions.text = ' '
    region = ET.SubElement(regions, 'region')
    region.text = ' '
    region.set('x1', str(x1))
    region.set('x2', str(x2))
    region.set('y1', str(y1))
    region.set('y2', str(y2))

def run(input_net_file, output_net_file, partitions_file, total_partitions, blocks, nx, ny):
    partitions = read_partitions(partitions_file)
    blocks = read_blocks(blocks)

    tree = ET.parse(input_net_file)
    root = tree.getroot()

    for child in root:
        if child.tag == "block":
            annotate_block_with_regions(blocks, child, partitions, total_partitions, nx, ny)

    tree.write(output_net_file)

def main():
    parser = argparse.ArgumentParser(description='Adds placement regions to VPR\'s net file based on hmetis output')
    parser.add_argument('--input-net-file', required=True)
    parser.add_argument('--output-net-file', required=True)
    parser.add_argument('--partitions', required=True)
    parser.add_argument('--total-partitions', required=True, type=int)
    parser.add_argument('--blocks', required=True)
    parser.add_argument('--nx', required=True, type=int)
    parser.add_argument('--ny', required=True, type=int)
    args = parser.parse_args()

    run(args.input_net_file, args.output_net_file, args.partitions, args.total_partitions, args.blocks, args.nx, args.ny)

if __name__ == "__main__":
    main()
