#!/usr/bin/env python
import argparse
import subprocess
import fileinput
import sys
import os
import re
import shutil
from collections import OrderedDict

def parse_args():
    parser = argparse.ArgumentParser(description="Restore Vanilla Latches to a Passed in BLIF File by replacing the BlackBoxed Latches")

    parser.add_argument("-b", 
                        type=str,
                        default="blackbox_latches.blif",
                        metavar="BLIF_FILE_TO_RESTORE",
                        help="The File Path to the BLIF File for Restorations of Vanilla Latches")

    parser.add_argument("-r", 
                        type=str,
                        default="restored.blif",
                        metavar="RESTORED_BLIF_FILE",
                        help="The File Path to the BLIF File Restored with Vanilla Latches")

    return parser.parse_args()

def main():

    args = parse_args();

    print "BLIF_FILE_TO_RESTORE: {}".format(args.b)
    print "RESTORED_BLIF_FILE: {}".format(args.r)

# TODO: Enable In Place Replacement End to End

    replacements = {'.subckt bb_latch':'.latch ', 'i0=':'', 'o0=':''}
    with open(args.b) as infile, open(args.r, 'w') as outfile:
        for line in infile:
            for src, target in replacements.iteritems():
                line = line.replace(src, target)
            outfile.write(line)

    ignore = False
    for line in fileinput.input(args.r, inplace=True):
        if not ignore:
            if line.startswith('.model bb_latch'):
                ignore = True
            else:
                print line,
        if ignore and line.isspace():
            ignore = False

    return

if __name__ == "__main__":
    main()
