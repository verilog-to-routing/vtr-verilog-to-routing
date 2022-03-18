#!/usr/bin/python

import os
import traceback
import sys
import re
import argparse
from xml.etree.ElementTree import tostring

# Arguments
ap = argparse.ArgumentParser(description='Parse information on internal connection of Switchbox')
ap.add_argument('-route_file', required=False, default='/home/vm/VTR-Tools/workspace/Test_Folder/Reference/blink/Reference_Arch/rr_graph2.xml', help='Location of input file containing routing info')
ap.add_argument('-out_file', required=False, default='/home/vm/VTR-Tools/workspace/Test_Folder/Reference/blink/Reference_Arch', help='Location of output file containing match info')
ap.add_argument('-help', required=False, help='List necessary arguments')
args = ap.parse_args()

# Initialization
regex_list = list()
x = 0
y = 0
route = None
line_number = 1

print("Route file in use: " + str(args.route_file) + "\nOutput file in use: " +  str(args.out_file))

out_file = open(args.out_file + '/out_file.txt',"w")

# Regex data structure
with open(args.route_file, "r") as route_file:

    # Sets values of Regex to use
    re_same_side_1 = re.compile('.+(?:(?P<chan>CHAN[XY]))\s+\((?:(?P<x1>\d+)),(?:(?P<y1>\d+))\)\s+to\s+\((?:(?P<x2>\d+)),(?:(?P<y2>\d+))\)')
    re_same_side_2 = re.compile('.+(?:(?P<chan>CHAN[XY]))\s+\((?:(?P<x1>\d+)),(?:(?P<y1>\d+))\)\s+\)')
    re_same_side_3 = re.compile('.+direction="(INC|DEC)_DIR.+id=\"(?:(?P<id>\d+))\".+type=\"(?:(?P<chan>CHAN[XY]))\".+ptc=\"(?:(?P<ptc>\d+))\".+xhigh\=\"(?:(?P<xh>\d+)).+xlow\=\"(?:(?P<xl>\d+)).+yhigh\=\"(?:(?P<yh>\d+)).+ylow\=\"(?:(?P<yl>\d+))')


    # Loops through line of the route file
    for line in route_file:
        # Finds regex matches
        match_same_side_1 = re.match(re_same_side_1, line)
        match_same_side_2 = re.match(re_same_side_2, line)
        match_same_side_3 = re.match(re_same_side_3, line)
        if match_same_side_1:
            x1 = int(match_same_side_1.group('x1'))
            y1 = int(match_same_side_1.group('y1'))
            x2 = int(match_same_side_1.group('x2'))
            y2 = int(match_same_side_1.group('y2'))
            #print(x1,x2,y1,y2)
            if x1 == x2 and y1 == y2:
                print("Match found with double coordinates")
        
        if match_same_side_2:
            chan = match_same_side_2.group('chan')
            #print(chan)
            if chan:
                print("Match found with single coordinates")
        
        if match_same_side_3:
            xh = int(match_same_side_3.group('xh'))
            xl = int(match_same_side_3.group('xl'))
            yh = int(match_same_side_3.group('yh'))
            yl = int(match_same_side_3.group('yl'))
            id = int(match_same_side_3.group('id'))
            chan = match_same_side_3.group('chan')
            ptc = int(match_same_side_3.group('ptc'))
            #print(x1,x2,y1,y2)
            if xh == xl and yh == yl:                
                out_file.write("Match found with double coordinates at line: " + str(line_number) + "\nid:" + str(id) + " type:" + chan + " ptc:" + str(ptc) + " xh:" + str(xh) + " xl:" + str(xl) + " yh:" + str(yh) + " yl:" + str(yl) + "\n\n")
        line_number += 1

