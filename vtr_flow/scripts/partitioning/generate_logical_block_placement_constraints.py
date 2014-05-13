import update_net_file

def run(output_partitions, partitions_file, total_partitions, nx, ny):
    partitions = update_net_file.read_partitions(partitions_file)

    with open(output_partitions, "w") as f:
        for idx in range(len(partitions)):
            x1 = 0
            x2 = nx + 1
            y1, y2 = update_net_file.partition_number_to_y_range(partitions[idx], total_partitions, ny)
            f.write("%d %d %d %d\n" % (x1, x2, y1, y2))
