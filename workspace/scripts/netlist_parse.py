#!/usr/bin/python

# -------------------------------------
# Description:  Uses .route file from Verilog-To-Routing to create a database of I/O connection
#               needed for each switchbox of an architecture. In addition, it can give a list 
#               of each wire in use in the design in a separate CSV.
# Arguments:    Takes a .route file as an input and outputs it to a specified CSV with the
#               possibility to store each information on each track of a connect box in a 
#               separate CSV. 
# Author:       Victor Marot (University of Bristol)  
# -------------------------------------

#Nomenclature:
# - CHANX (a,b) -> Horizontal wires with "a" ... and "b" the location of the CB tile vertically (down to up)
# - CHANY (a,b) -> Vertical wires with "a" the horizontal location of the CB tile (left to right) and "b" ...
# - len x -> Wire spread over x CB/SB combination (eg. x=2 means wire is 2 CB and 2 SB long, x=1 is 1 CB and 1 SB long, x=0 is 1 CB and 0 SB (just the length of the connect box the wire belongs to))
# - Maybe use longline to help locate get the missing values

#TODO: Need to modify for CB:
#       - CB cannot be located by track but rather by pins
#       - Need to input architecture to know which pin is connected to which track

import os
import traceback
import sys
import pandas as pd
import numpy as np
import re
import argparse
import tkinter

# Arguments
ap = argparse.ArgumentParser(description='Parse information on internal connection of Switchbox')
ap.add_argument('-route_in', required=True, default='/home/vm/VTR-Tools/vtr_work/quickstart/blink.route', help='Location of input file containing routing info')
ap.add_argument('-regex_out', required=False, default=None, help='Location of output file containing regex info from route_in')
ap.add_argument('-route_out', required=True, default='/home/vm/VTR-Tools/vtr_work/quickstart/switchbox_routing.csv', help='Location of output file containing processed info')
ap.add_argument('-help', required=False, help='List necessary arguments')
args = ap.parse_args()

# Initialization
regex_list = list()
x = 0
y = 0
route = None

# Regex data structure
with open(args.route_in, "r") as route_file:

    # Sets values of Regex to use
    re_matrix = re.compile('Array\s+size:\s+(?:(?P<x>[0-9]+))\s+x\s+(?:(?P<y>[0-9]+))')
    re_net = re.compile('Net\s+(?:(?P<net>[0-9]+))')
    re_short = re.compile('.+(?:(?P<chan>CHAN[XY]))\s+\((?:(?P<x1>\d+)),(?:(?P<y1>\d+))\)\s+Track:\s+(?:(?P<track>\d+))')
    re_long = re.compile('.+(?:(?P<chan>CHAN[XY]))\s+\((?:(?P<x1>\d+)),(?:(?P<y1>\d+))\)\s+to\s+\((?:(?P<x2>\d+)),(?:(?P<y2>\d+))\)\s+Track:\s+(?:(?P<track>\d+))')
    re_pin = re.compile('.+[OI]PIN\s+\((?:(?P<xpin>[0-9]+)),(?:(?P<ypin>[0-9]+))\)\s+Pin:\s+(?:(?P<pin>[0-9]+))')


    # Loops through line of the route file
    for line in route_file:

        # Finds regex matches
        match_matrix = re.match(re_matrix, line)
        match_net = re.match(re_net, line)
        match_short = re.match(re_short, line)
        match_long = re.match(re_long, line)
        match_pin = re.match(re_pin, line)

        #print(match_matrix)

        # Stores matched value of matrix size
        if match_matrix:
            matrix_size = (int(match_matrix.group('x')),int(match_matrix.group('y')))

        # Stores matched value of net
        if match_net:
            net = int(match_net.group('net'))

        # Stores matched coordinates of single CB wires
        if match_short:
            chan = match_short.group('chan')
            x1 = int(match_short.group('x1'))
            y1 = int(match_short.group('y1'))
            track = int(match_short.group('track'))
            regex_list.append([net,chan,track,x1,None,y1,None])

        # Stores matched coordinates of multi CB wires
        if match_long:
            chan = match_long.group('chan')
            x1 = int(match_long.group('x1'))
            y1 = int(match_long.group('y1'))
            x2 = int(match_long.group('x2'))
            y2 = int(match_long.group('y2'))
            track = int(match_long.group('track'))
            regex_list.append([net,chan,track,x1,x2,y1,y2])

        # Stores matched pin number for CB
        if match_pin:
            pin = int(match_pin.group('pin'))
            x_pin_loc = int(match_pin.group('xpin'))
            y_pin_loc = int(match_pin.group('ypin'))    

# Update dataframe and save it to CSV
sb_regex_df = pd.DataFrame(data=regex_list, columns=('net','type','track','x1','x2','y1','y2'))
cb_regex_df = pd.DataFrame(data=regex_list, columns=('net','pin','x','y'))
if args.regex_out:
    sb_regex_df.to_csv(args.regex_out+"_SB")
    cb_regex_df.to_csv(args.regex_out+"_CB")

# Create dataframe for switchbox list
print(match_matrix)
print(matrix_size)
sb_df = pd.DataFrame(index=range(matrix_size[0]*matrix_size[1]), columns=('x','y'))
cb_df = pd.DataFrame(index=range(matrix_size[0]*matrix_size[1]), columns=('x','y')) 


# Add location index of FPGA tiles according to the design size
for i in range(matrix_size[0]*matrix_size[1]):
    sb_df['x'].loc[i] = x
    sb_df['y'].loc[i] = y
    x += 1
    if x > (matrix_size[0]-1):
        y += 1
        x = 0

# Loops through net routed
for net in sb_regex_df['net'].unique():
    index_tile = 0

    # Loops through all coordinates of tiles in the architecture
    for _,tile_xy in sb_df.iterrows(): 
        column_name = 0

        # Loops through all connections of a single net
        for _,iteration_connection in sb_regex_df[sb_regex_df['net'] == net].iterrows(): # track is the column of the data to use, net is the column of the data to find
            route = None
            # List all possiblities            
            if not pd.isnull(iteration_connection['x2']) and not pd.isnull(iteration_connection['y2']):

            # Case 1: Net is going straight and horizontaly through the switch box
                # Find if there is current SB(x,_) in the current net #
                if iteration_connection['x1'] <= tile_xy['x'] and iteration_connection['x2'] > tile_xy['x'] and iteration_connection['type'] == 'CHANX' and iteration_connection['y1'] == tile_xy['y']:
                    
                    # Defines used e-w tracks to achieve right connection
                    route = 'e%i' % iteration_connection['track'] + ' w%i' % iteration_connection['track']
            
            # Case 2: Net is going straight and vertically through the switch box
                # Find if there is current SB(_,y) in the current net #
                if iteration_connection['y1'] <= tile_xy['y'] and iteration_connection['y2'] > tile_xy['y'] and iteration_connection['type'] == 'CHANY' and iteration_connection['x1'] == tile_xy['x']:
                    
                    # Define used e-w tracks to achieve right connection
                    route = 'n%i' % iteration_connection['track'] + ' s%i' % iteration_connection['track']
            
            else:
            # Case 3: Net is connected to a N track
                if iteration_connection['y1'] == (tile_xy['y']+1) and iteration_connection['type'] == 'CHANY' and iteration_connection['x1'] == tile_xy['x']:
                    route = 'n%i' % iteration_connection['track']

            # Case 4: Net is connected to a E track
                if iteration_connection['x1'] == (tile_xy['x']+1) and iteration_connection['type'] == 'CHANX' and iteration_connection['y1'] == tile_xy['y']:
                    route = 'e%i' % iteration_connection['track']

            # Case 5: Net is connected to a W track
                if iteration_connection['x1'] == tile_xy['x'] and iteration_connection['type'] == 'CHANX' and iteration_connection['y1'] == tile_xy['y']:
                    route = 'w%i' % iteration_connection['track']

            # Case 6: Net is connected to a S track
                if iteration_connection['y1'] == tile_xy['y'] and iteration_connection['type'] == 'CHANY' and iteration_connection['x1'] == tile_xy['x']:
                    route = 's%i' % iteration_connection['track']
        
# NOTE: If a net column of switchbox_routing.csv only has 1 connection defined, it means that the pin is not in use by the switchbox

            # Finds if there is a route to write
            if route is not None:
                
                # Creates columns if inexistant
                if net not in sb_df.columns:
                    sb_df[net] = np.nan

                # Stores route values
                if not pd.isnull(sb_df[net].loc[index_tile]):
                    sb_df[net].loc[index_tile] = np.char.add(sb_df[net].loc[index_tile], ' '  + route)
                else: 
                    sb_df[net].loc[index_tile] = route
        
        # Increment tile index used to store value in the correct place
        index_tile += 1


# Output list of switchbox
sb_df.to_csv(args.route_out)
