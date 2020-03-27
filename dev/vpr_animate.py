#!/usr/bin/env python3

import argparse
import sys

def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("vpr")
    parser.add_argument("arch")
    parser.add_argument("circuit")
    parser.add_argument("--placements", nargs="+")
    parser.add_argument("--routings", nargs="+")
    parser.add_argument("--graphics_commands")

    return parser.parse_known_args()

def main():
    args, extra_args = parse_args()

    vpr_base_cmd = "{} {} {}".format(args.vpr, args.arch, args.circuit)

    for arg in extra_args:
        vpr_base_cmd += " " + arg

    i = 0
    for placement in args.placements:
        cmd = vpr_base_cmd + " --place_file " + placement + " --route"
        cmd += " --graphics_commands '{}'".format(args.graphics_commands.format(i=i))
        cmd += " >& vpr_img_{}.log".format(i)
        print(cmd)
        i += 1

    for routing in args.routing:
        cmd = vpr_base_cmd + " --place_file " + args.placement[-1] + " --route_file " + routing + "--analysis"
        cmd += " --graphics_commands '{}'".format(args.graphics_commands.format(i=i))
        cmd += " >& vpr_img_{}.log".format(i)
        print(cmd)
        i += 1


if __name__ == "__main__":
    main()
