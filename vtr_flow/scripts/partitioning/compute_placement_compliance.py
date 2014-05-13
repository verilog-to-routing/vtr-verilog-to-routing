import sys
import xml.etree.ElementTree as ET

def read_place_file(pf):
    placement = {}

    with open(pf) as f:
        skip_lines = 5
        for line in f:
            if skip_lines > 0:
                skip_lines -= 1
                continue

            parts = line.strip().split()
            block_name = parts[0]
            x = int(parts[1])
            y = int(parts[2])
            placement[block_name] = (x,y)

    return placement

def run(place_file, partitioned_netlist_file):
    placement = read_place_file(place_file)

    tree = ET.parse(partitioned_netlist_file)
    root = tree.getroot()

    count_good = 0
    count_bad = 0

    for child in root:
        if child.tag == "block":
            block_name = child.get("name")
            region = child.find("regions").find("region")
            x1 = int(region.get("x1"))
            x2 = int(region.get("x2"))
            y1 = int(region.get("y1"))
            y2 = int(region.get("y2"))
            actual_position = placement[block_name]
            if actual_position[0] >= x1 and actual_position[0] <= x2 and actual_position[1] >= y1 and actual_position[1] <= y2:
                count_good += 1
            else:
                count_bad += 1

    return (count_good, count_bad)

def main():
    place_file = sys.argv[1]
    partitioned_netlist_file = sys.argv[2]
    count_good, count_bad = run(place_file, partitioned_netlist_file)
    print "%d good, %d bad" % (count_good, count_bad)

if __name__=="__main__":
    main()
