#!/usr/bin/python3

import os
import traceback
import sys
import pandas as pd
import numpy as np
import re
import argparse

# Arguments
ap = argparse.ArgumentParser(description='Give internal switch position for different topologies')
ap.add_argument('-route', required=False, default='/home/vm/VTR-Tools/workspace/scripts/switchbox_routing.csv', help='Location of input file containing routing info')
ap.add_argument('-sbu', required=False, default='/home/vm/VTR-Tools/workspace/scripts/SB-U_Positions.csv', help='Location of output file containing SB-U info')
ap.add_argument('-ld', required=False, action="store_true", help='Use -sbu to load previous file')
args = ap.parse_args()

position = pd.read_csv(args.route, index_col=0)
tot_track = 100



#TODO: Create empty dataframe to store SBU info afterwards

if args.ld:
    # Loads data from previous CSV  
    sbu_df = pd.read_csv(args.sbu, header=[0, 1], index_col=0)

else:
    # Creates a new df with SB coordinates and number of SB-U in each SB
    sbu_df = pd.DataFrame(0b00, index=range(position.index.size), columns=pd.MultiIndex.from_product([[str(i) for i in range(tot_track)], ['N','E','S','W']])) # Creates SB-U range with sub-columns (also keeps data as string)
    df_test = pd.DataFrame(0, index=range(position.index.size), columns=pd.MultiIndex.from_product([['x','y'], ['']]))                               # Create null coordinates range with empty sub-columns
    df_test['x'],df_test['y'] = position['x'],position['y']                                                                                          # Adds coodinates in coordinate range
    sbu_df = pd.concat([df_test,sbu_df], axis=1)                                                                                                     # Concatenate SB-U range and coordinate range 

sb_type = 'disjoint'

# Regex
re_all = re.compile('(?:(?P<pole>[nesw]))(?:(?P<track>[0-9]+))')
re_N = re.compile('[n](?:(?P<track>[0-9]+))')
re_E = re.compile('[e](?:(?P<track>[0-9]+))')
re_W = re.compile('[s](?:(?P<track>[0-9]+))')
re_S = re.compile('[w](?:(?P<track>[0-9]+))')


def pole(info):
    re_pole = re.compile('')
    match_pole = re.match(re_pole, info)
    return match_pole

def track(info):
    re_track = re.compile('')
    match_track = re.match(re_track, info)
    return match_track

# Input NESW and current state for given SBU, use OR to modify the SBU value in dataframe
def switch(track, north, east, south, west):
    # 00 -> neutral
    # 01 -> anti-clockwise
    # 10 -> clockwise

    if not north and not east and not south and not west:
        return [0b00,0b00,0b00,0b00]
    if not north and not east and south and west:
        return [0b00,0b00,0b10,0b01]
    if not north and east and not south and west:
        return [0b00,0b10,0b00,0b10]
    if not north and east and south and not west:
        return [0b00,0b01,0b10,0b00]
    if not north and east and south and west:
        return [0b00,0b10,0b01,0b10]
    if north and not east and not south and west:
        return [0b01,0b00,0b00,0b10]
    if north and not east and south and not west:
        return [0b10,0b00,0b10,0b00]
    if north and not east and south and west:
        return [0b10,0b00,0b10,0b01]
    if north and east and not south and not west:
        return [0b01,0b10,0b00,0b00]
    if north and east and not south and west:
        return [0b01,0b10,0b00,0b10]
    if north and east and south and not west:
        return [0b10,0b01,0b10,0b00]
    if north and east and south and west:
        return [0b10,0b01,0b10,0b01]

    # sbu = LUT[pole]
    return [0b00,0b00,0b00,0b00]

# TODO: Make it flexible. The 'If sb_type' has to include different topologies but still be based on the same inputs and same SB-U.
# Don't hesitate to assume tracks values -> for Disjoint each NESW has the same track number

#print(switch(False,False,True,True))

# For Disjoint topology
if sb_type == 'disjoint':
    
    #TODO: Cycle through tracks and apply function for each switch box
    #Increase value of parallel variable to say that parallel connections are required
    
    # FORMAT: For the table -> Columns = SBU number (aka same as track #), Rows = SB address
    # Use SBU to store in dataframe, input into switch(), modify it, return to dataframe
    info = 0b0111   # Info on pole from regex
    track = 5
    
    # TODO: Find all match of cell in one variable then do 'find' in the variable to group by track values

    for coordinate in sbu_df.index:

        # Do the regex to determine track number to be used and instead loop through net number

        # TODO: Loop through Net number of input file(NOT TRACKS)
        for net in position.columns:            
            if net != 'x' and net != 'y' and pd.notnull(position[net].loc[coordinate]):
                
                # Checks if the SB-U is already in use for another connection
                #if sbu_df[str(track), 'N'].loc[coordinate] or sbu_df[str(track), 'E'].loc[coordinate] or sbu_df[str(track), 'S'].loc[coordinate] or sbu_df[str(track), 'W'].loc[coordinate]:
                if 1:
                    # Match
                    match_N = re.match(re_N, position[net].loc[coordinate])
                    match_E = re.match(re_E, position[net].loc[coordinate])
                    match_S = re.match(re_S, position[net].loc[coordinate])
                    match_W = re.match(re_W, position[net].loc[coordinate])
                    match_all_list = re.findall(re_all, position[net].loc[coordinate])
                    match_all_array = np.array(match_all_list)

                    for track in np.unique(match_all_array[:,1]):                        

                        # Example: Print True if S is found for the current each track value
                        #print(('s', track) in match_all_list)
                        #TODO: Make list with all tracks 
                        
                        #match_all_list = [('e', '28'),('n','28'),('w','28')]
                        #print(match_all_list)
                        north = ('n', track) in match_all_list
                        east = ('e', track) in match_all_list
                        south = ('s', track) in match_all_list
                        west = ('w', track) in match_all_list
                        #print(north,east,south,west)

                        # Modify current SB-U switch position
                        sbu_pos = switch(track,north, east, south, west)                        
                        #print(sbu_pos)

                        # Add condition if switchs are already used
                        if not sbu_df[str(track), 'N'].loc[coordinate] and not sbu_df[str(track), 'E'].loc[coordinate] and not sbu_df[str(track), 'S'].loc[coordinate] and not sbu_df[str(track), 'W'].loc[coordinate]:
                            print('test')
                        elif 1:
                            print('fuck')
                        else:
                            sbu_df[str(track), 'N'].loc[coordinate] = sbu_pos[0]
                            sbu_df[str(track), 'E'].loc[coordinate] = sbu_pos[1]
                            sbu_df[str(track), 'S'].loc[coordinate] = sbu_pos[2]
                            sbu_df[str(track), 'W'].loc[coordinate] = sbu_pos[3]        

# Save switches positions
sbu_df.to_csv(args.sbu)
