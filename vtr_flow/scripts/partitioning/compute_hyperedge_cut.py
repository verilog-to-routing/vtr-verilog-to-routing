import sys

def read_partitions(pf):
    partitions = []
    with open(pf) as f:
        for line in f:
            partitions.append(int(line.strip()))

    return partitions

def process_hypergraph(partitions, hgf):
    first_line = True
    cutsize = 0
    with open(hgf) as f:
        for line in f:
            if first_line:
                first_line = False
                continue
            comment_idx = line.find("//")
            if comment_idx > -1:
                line = line[0:comment_idx]
            line = line.strip()
            if len(line) == 0:
                continue
            # -1 to go to 0-indexed
            vertices_in_hyperedge = map(lambda x: int(x)-1, line.split())
            partitions_spanned_by_hyperedge = set(map(lambda x: partitions[x], vertices_in_hyperedge))
            if len(partitions_spanned_by_hyperedge) > 1:
                cutsize += 1
    return cutsize

def run(hypergraph_file, partition_file):
    partitions = read_partitions(partition_file)
    return process_hypergraph(partitions, hypergraph_file)

def main():
    hypergraph_file = sys.argv[1]
    partition_file = sys.argv[2]

    print run(hypergraph_file, partition_file)

if __name__=="__main__":
    main()
