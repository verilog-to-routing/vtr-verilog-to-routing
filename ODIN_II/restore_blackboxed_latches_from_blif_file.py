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
    global parser

    parser = argparse.ArgumentParser(description="Restore Vanilla Latches to a Passed in BLIF File by replacing the BlackBoxed Latches")

    parser.add_argument("blifFileToRestore",
                        type=str,
                        metavar="BLIF_FILE_TO_RESTORE",
                        help="The File Path to the BLIF File for Restoration of Vanilla Latches")
    group = parser.add_mutually_exclusive_group()

    group.add_argument("-i",
                        "--inplace",
                        action="store_true",
                        help="Restore to the Passed in BLIF File Inplace")

    group.add_argument("-r",
                        "--restoredBlifFile",
                        type=str,
                        metavar="RESTORED_BLIF_FILE",
                        help="The File Path to the BLIF File Restored with Vanilla Latches")

    return parser.parse_args()

def main():
    global parser

    args = parse_args();

    # print "DEBUG: BLIF_FILE_TO_RESTORE: {}".format(args.blifFileToRestore)
    # print "DEBUG: INPLACE: {}".format(args.inplace)
    # print "DEBUG: RESTORED_BLIF_FILE: {}".format(args.restoredBlifFile)

    if not args.inplace:
        print "\n\nERROR: Must Specify Either Inplace Restoration \"-i,--inplace\" or a file to restore to \"-r\--restoredBlifFile\"\nExiting...\n"
        parser.print_help()
        return -1

    if not os.path.isfile(args.blifFileToRestore):
        print "\n\nERROR: BLIF File to Restore \"{}\" doesn not exist\nExiting...\n".format(args.blifFileToRestore)
        parser.print_help()
        return -1

    # print "Restoring original inputs, ouptuts, types, controls and init_vals:"

    if args.inplace:
        args.restoredBlifFile = "restored.blif.tmp"
    pair_matching_replace = {}
    previous = False
    is_slash = False
    my_token = ""
    with open(args.blifFileToRestore) as infile, open(args.restoredBlifFile, 'w') as outfile:
        for line in infile:
            if is_slash is True:
                is_slash = False
                line = line.strip()
                tokens = line.split()
                pair_matching_replace[tokens[0]] = my_token

            elif previous is True:
                previous = False

            elif '.names bb_latch_' in line:
                line = line.strip()
                tokens = line.split()
                previous = True
                my_token = tokens[1]

                if '\\' in  tokens[2]:
                    is_slash = True  
                else:
                    pair_matching_replace[tokens[2]] = tokens[1]

            else:
                outfile.write(line)

    if args.inplace:
        src_filename = os.path.join(args.restoredBlifFile)
        dest_filename = os.path.join(args.blifFileToRestore)
        shutil.move(src_filename, dest_filename)

    bs = False
    with open(args.blifFileToRestore) as infile, open(args.restoredBlifFile, 'w') as outfile:
        for line in infile:
            if  'subckt bb_latch' in line:
                bs = True
            else:
                outfile.write(line)

    if args.inplace:
        src_filename = os.path.join(args.restoredBlifFile)
        dest_filename = os.path.join(args.blifFileToRestore)
        shutil.move(src_filename, dest_filename)

    with open(args.blifFileToRestore) as infile, open(args.restoredBlifFile, 'w') as outfile:
        for line in infile:
            if '.latch' in line:
                line = line.strip()
                tokens = line.split()
                null_check = pair_matching_replace.get(tokens[1], "")
                if null_check is not "":
                    tokens[1] = pair_matching_replace[tokens[1]]
                    # bb_latch_@top^clk@re@3@top^MULTI_PORT_MUX~11618^MUX_2~50242@1
                    # bb_latch_ top^clk re 3 top^MULTI_PORT_MUX~11618^MUX_2~50242 1
                    latch_information = tokens[1].split("@")
                    line = tokens[0] + " " + latch_information[4] + " " + tokens[2] + " " + latch_information[2] + " " + latch_information[1] + " " + latch_information[3] + "\n"

            outfile.write(line)

    # print "Removing BlackBoxed Latch Model:"

    ignore = False
    for line in fileinput.input(args.restoredBlifFile, inplace=True):
        if not ignore:
            if line.startswith('.model bb_latch'):
                ignore = True
            else:
                print line,
        if ignore and line.isspace():
            ignore = False

    if args.inplace:
        src_filename = os.path.join(args.restoredBlifFile)
        dest_filename = os.path.join(args.blifFileToRestore)
        shutil.move(src_filename, dest_filename)

    #     print "BLIF File Restored. See: {}".format(args.blifFileToRestore)
    # else:
    #     print "BLIF File Restored. See: {}".format(args.restoredBlifFile)

    return

if __name__ == "__main__":
    main()
