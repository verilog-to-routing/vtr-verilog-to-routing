#!/usr/bin/env python
import sys
import matplotlib.pyplot as plt
import numpy as np
import re
from collections import OrderedDict
import argparse

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("log_file", help="Log file to process")
    parser.add_argument("--metric", choices=["edge_src_node", "traversed_node", "traversed_edge", "node_tags"])

    args = parser.parse_args()

    return args



def main():
    args = parse_args()

    data = []
    arrival_level_indexes = OrderedDict()
    required_level_indexes = OrderedDict()

    data_regex = None
    if(args.metric == "edge_src_node"):
        data_regex = re.compile(r"Edge\((?P<edge_id>\d+)\) src_node")
    elif (args.metric == "traversed_node"):
        data_regex = re.compile(r"(Arrival|Required) Traverse Node\((?P<id>\d+)\)")
    elif (args.metric == "traversed_edge"):
        data_regex = re.compile(r"(Arrival|Required) Traverse Edge\((?P<id>\d+)\)")
    elif (args.metric == "node_tags"):
        data_regex = re.compile(r"Tags Node\((?P<id>\d+)\)")
    else:
        assert False

    start_traversals_regex = re.compile(r"Start Traversals")
    end_traversals_regex = re.compile(r"End Traversals")
    arrival_level_regex = re.compile(r"Arrival Level\((?P<level_id>\d+)\)")
    required_level_regex = re.compile(r"Required Level\((?P<level_id>\d+)\)")

    with open(sys.argv[1]) as f:
        i = 0
        in_traversals = False
        for line in f:
            match = start_traversals_regex.match(line)
            if match:
                in_traversals = True
            match = end_traversals_regex.match(line)
            if match:
                in_traversals = False
                break

            if in_traversals:
                match = data_regex.match(line)
                if match:
                    data.append(int(match.group("id")))
                    i += 1

                match = arrival_level_regex.match(line)
                if match:
                    arrival_level_indexes[i] = int(match.group("level_id"))

                match = required_level_regex.match(line)
                if match:
                    required_level_indexes[i] = int(match.group("level_id"))

            #if i > 1000:
                #break


    max_value = max(data)

    num_values = len(data)

    print "Max: ", max_value
    print "num values: ", num_values

    y = range(num_values)
    x = data


    for access_number, level_id in arrival_level_indexes.iteritems():
        print "Arrival Level", level_id, " at access ", access_number
        plt.axhline(access_number, color="g", alpha=0.25)

    for access_number, level_id in required_level_indexes.iteritems():
        print "Required Level", level_id, " at access ", access_number
        plt.axhline(access_number, color="r", alpha=0.25)

    plt.scatter(x, y)



    plt.xlabel("Array Index " + args.metric)
    plt.ylabel("Time (access #)")
    plt.xlim(0, max_value)
    plt.ylim(0, num_values)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
