#!/usr/bin/env python
import sys
import os

if __name__ == "__main__":
    if len(sys.argv) is not 2:
        print "\n\nERROR: no file name! \nExiting...\n"
        exit()
        
    blif_file_name=sys.argv[1]
        
    if not os.path.isfile(blif_file_name):
        print "\n\nERROR: BLIF File to Restore \"{}\" doesn not exist\nExiting...\n".format(blif_file_name)
        exit()

    pair_matching_replace = {}
    previous = False
    ignore = False
    my_token = ""
    with open(blif_file_name,'r') as infile, open('temp_bb.blif', 'w') as outfile:
        for line in infile:
            # remove all .model bb_latch lines
            if not ignore:
                if line.startswith('.model bb_latch'):
                    ignore = True
                
                #remove all truth table for .names bb_latch    
                elif previous is True:
                    previous = False
                
                # parse .names bb_latch and fetch info and ref name nxxxxx.. in dictionary for matching
                # and replace
                elif '.names bb_latch_' in line:
                    line = line.strip()
                    tokens = line.split()
                    previous = True
                    my_token = tokens[1]
                    pair_matching_replace[tokens[2]] = tokens[1]
                
                elif  'subckt bb_latch' not in line:
                    outfile.write(line)
                    
            else:
                if line.isspace():
                    ignore = False

    with open('temp_bb.blif', 'r') as infile, open(blif_file_name,'w') as outfile:
        for orig_line in infile:
            if '.latch' in orig_line:
                line = orig_line.strip()
                tokens = line.split()
                
                # check if we have this cube name in our table
                null_check = pair_matching_replace.get(tokens[1], "")
                if null_check is not "":
                    tokens[1] = pair_matching_replace[tokens[1]]
                    # bb_latch_@top^clk@re@3@top^MULTI_PORT_MUX~11618^MUX_2~50242@1
                    # bb_latch_ top^clk re 3 top^MULTI_PORT_MUX~11618^MUX_2~50242 1
                    latch_information = tokens[1].split("@")
                    orig_line = tokens[0] + " " + latch_information[4] + " " + tokens[2] + " " + latch_information[2] + " " + latch_information[1] + " " + latch_information[3] + "\n"

            outfile.write(orig_line)
    exit()
