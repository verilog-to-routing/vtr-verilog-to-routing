#!/usr/bin/env python3

"""
    Module for running tests to verify the NoC placement of vpr
"""

from cgi import test
from ctypes.wintypes import FLOAT
from distutils.log import error
from email.headerregistry import Address
from fileinput import close
import os
from pathlib import Path
import string
import sys
import argparse
import textwrap
import subprocess
import re as regex
import csv
from multiprocessing import Pool, Manager
from operator import itemgetter
import smtplib
from email.message import EmailMessage

sys.path.insert(0, str(Path(__file__).resolve().parent / "vtr_flow/scripts/python_libs"))
sys.path.insert(0, str(Path(__file__).resolve().parent / "vtr_flow/scripts"))

from vtr import RawDefaultHelpFormatter, paths, vpr

# important extensions
CIRCUIT_FILE_EXT = ".blif"
FLOWS_FILE_EXT = ".flows"
CSV_FILE_EXT = ".csv"
NET_FILE_EXT = ".net"

# informat parsed from the vpr output
PLACE_COST = "place_cost"
PLACE_TIME = "place_time"
PLACE_BB_COST = "bb_cost"
PLACE_TD_COST = "td_cost"
NOC_AGGREGATE_BANDWIDTH_COST = "noc_aggregate_bandwidth_cost"
NOC_LATENCY_COST = "noc_latency_cost"
NOC_PLACEMENT_WEIGHT = "noc_placement_weight"

#phrases to identify lines that contain palcement data
PLACEMENT_COST_PHRASE = "Placement cost:"
NOC_PLACEMENT_COST_PHRASE = "NoC Placement Costs."
PLACEMENT_TIME = "# Placement took"

# regex match strings to extract placement info
PLACEMENT_COST_REGEX = 'Placement cost: (.*), bb_cost: (.*), td_cost: (.*),'
NOC_PLACEMENT_COST_REGEX = 'NoC Placement Costs. noc_aggregate_bandwidth_cost: (.*), noc_latency_cost: (.*),'
PLACEMENT_TIME_REGEX = '# Placement took (.*) seconds.*'

# global synhronization datastructures
main_manager = Manager()
design_run_data = main_manager.list()


def noc_test_command_line_parser(prog=None):
    """Parses the arguments of noc_benchmark_test"""

    description = textwrap.dedent(
        """
                    Runs VPR placer on one or more NoC benchmark designs.
                    Runs VPR multiple times on each circuit and collects
                    placement statistics. Each VPR run will use a different
                    value for the the NoC placement weighting. The objective 
                    is the identify an appropriate default NoC weighting value.
                    """
    )
    epilog = textwrap.dedent(
        """
                Examples
                --------

                    Run the NoC driven placement on a design located at ./noc_test_circuits (design should be in .blif format). Where we want to see the NoC weighting go from 0-100 at intervals of 1:

                        %(prog)s -tests_folder_location ./noc_test_cricuits -max_weighting 100 -interval 1
 
                """
    )

    parser = argparse.ArgumentParser(
        prog=prog,
        description=description,
        epilog=epilog,
        formatter_class=RawDefaultHelpFormatter,
    )

    #
    # Important Arguments
    #
    parser.add_argument(
        "-tests_folder_location",
        default="",
        type=str,
        help="Path to the location of the test designs"
    )

    parser.add_argument(
        "-max_weighting",
        default=1,
        type=float,
        help="The highest weight value used for the NoC placement cost"
    )

    parser.add_argument(
        "-interval",
        default=1,
        type=float,
        help="The change in weighting between tests for the NoC placement cost"
    )

    parser.add_argument(
        "-arch_file",
        default = "",
        type=str,
        help="The architecture file the NoC benchamrk designs are placed on"
    )

    parser.add_argument(
        "-vpr_executable",
        default="",
        type=str,
        help="The executable file of VPR"
    )


    return parser

def find_all_circuit_files(location_of_designs, design_files, design_names, design_flow_files):

    # find all the circuit files located within the provided folder
    for single_design_file_path in Path(location_of_designs).rglob('*{0}'.format(CIRCUIT_FILE_EXT)):
        # store the design
        design_files.append(single_design_file_path)

        # find and store the name of the design
        design_names.append(Path(single_design_file_path).stem)
        
        # now we need to create the path to the corresponding design flows file
        # we expect this file to be in the same location as the design file and having the same name
        # So we just need to change the file extension to match what a flows file would have
        design_flow_files.append((os.path.splitext(single_design_file_path)[0]) + FLOWS_FILE_EXT)
    
    return

def process_vpr_output(vpr_output_file):

    open_file = open(vpr_output_file)

    #datastrcuture below stors the palcement data in a disctionary
    placement_data = {}

    # process each line from the VPR output
    for line in open_file:  
        
        # we only care about three lines where the placement costs, noc costs and placement times are located
        # so identify those lines below
        if PLACEMENT_COST_PHRASE in line:
            process_placement_costs(placement_data, line)

        if NOC_PLACEMENT_COST_PHRASE in line:
            process_noc_placement_costs(placement_data, line)

        if PLACEMENT_TIME in line:
            process_placement_time(placement_data, line)
    
    # close and delete the output file
    open_file.close()
    os.remove(vpr_output_file)

    return placement_data


def process_placement_costs(placement_data, line_with_data):

    # extract the placement costs from the line where they are located
    found_placement_metrics = regex.search(PLACEMENT_COST_REGEX, line_with_data)

    # quick check that the regex worked properly
    if found_placement_metrics.lastindex != 3:
        raise Exception("Placement cost not written out correctly")

    # we know the order of the different placement costs as they are found within the extracted metric list above
    # 1st element is the overall placement cost, second element is the placement bb cost and the third element is the placement td cost
    # covert them to floats and store them
    placement_data[PLACE_COST] = float(found_placement_metrics.group(1))
    placement_data[PLACE_BB_COST] = float(found_placement_metrics.group(2))
    placement_data[PLACE_TD_COST] = float(found_placement_metrics.group(3))
    
    return

def process_noc_placement_costs(placement_data, line_with_data):

    # extract the noc placement costs from the line where they are located
    found_placement_metrics = regex.search(NOC_PLACEMENT_COST_REGEX, line_with_data)

    # quick check that the regex worked properly
    if found_placement_metrics.lastindex != 2:
        raise Exception("Placement noc cost not written out correctly")

    # we know the order of the different noc placement costs as they are found within the extracted metric list above
    # 1st element is the noc aggregate bandwidth cost and the second element is the noc latency cost
    placement_data[NOC_AGGREGATE_BANDWIDTH_COST] = float(found_placement_metrics.group(1))
    placement_data[NOC_LATENCY_COST] = float(found_placement_metrics.group(2))
    
    return

def process_placement_time(placement_data, line_with_data):

    # extract the placement time from the line where it is located
    found_placement_metrics = regex.search(PLACEMENT_TIME_REGEX, line_with_data)

    # quick check that the regex worked properly
    if found_placement_metrics.lastindex != 1:
        raise Exception("Placement time not written out correctly")

    # there should be only one element, since we are only grabbing the placement time
    placement_data[PLACE_TIME] = float(found_placement_metrics.group(1))
    
    return

def execute_vpr_and_process_output(vpr_location, arch_file, design_file, design_flows_file, routing_algorithm, device, noc_placement_weight, vpr_out_file, vpr_net_file):
    # create the filr that will store the VPR output
    vpr_output = open(vpr_out_file, 'w')

    # run vpr. Will timeout after 30 minutes (maybe we need more for larger designs
    result = subprocess.run([vpr_location, arch_file, str(design_file), '--noc', 'on', '--noc_flows_file', design_flows_file, '--noc_routing_algorithm', routing_algorithm, '--device', device, '--noc_placement_weighting', str(noc_placement_weight), '--pack', '--place', '--net_file', vpr_net_file],check=True, stdout=vpr_output, timeout=1800)

    #close the output file
    vpr_output.close()
        
    # get the placement metrics for the current run
    curr_vpr_place_data = process_vpr_output(vpr_out_file)
    # indicate what the current noc weighting is 
    curr_vpr_place_data[NOC_PLACEMENT_WEIGHT] = noc_placement_weight

    # store the current run data
    design_run_data.append(curr_vpr_place_data)

    # delete the netfile since we dont need it anymore
    os.remove(vpr_net_file)

def run_vpr(vpr_location, arch_file, design_file, design_flows_file, max_noc_weighting, noc_weighting_interval):

    # setup command line parameters for vpr, these all can be paramitizable
    routing_algorithm = "xy_routing"
    device = "EP4SGX110"

    # now we need to generate a list of tuples that hold all the command line parameters
    # With each set of command line parameters, the temporary output file and the noc weighting will change
    test_args = []

    noc_weighting = 0.0
    while noc_weighting <= max_noc_weighting:
        # create the current output file name
        vpr_out_file = '{0}.{1}.vpr.out'.format(design_file, noc_weighting)

        #create the net file name
        vpr_net_file = '{0}.{1}{2}'.format(design_file, noc_weighting, NET_FILE_EXT)

        # generate the command tuple
        test_args.append((vpr_location, arch_file, design_file, design_flows_file, routing_algorithm, device, noc_weighting, vpr_out_file, vpr_net_file))

        noc_weighting+=noc_weighting_interval

    # create the multiprocessing pool
    execute_pool = Pool(3)
    # run each test run in a seperate process
    execute_pool.starmap(execute_vpr_and_process_output, test_args)

    # wait for all runs to finish
    execute_pool.close()
    execute_pool.join()



def write_csv_file(data, csv_file_name):
    # create the csv file name
    csv_file = csv_file_name + CSV_FILE_EXT

    # star by sorting the list of runs by their noc weighting value (ascending order)
    sorted_data = sorted(data, key=itemgetter(NOC_PLACEMENT_WEIGHT))

    # open the csv file and write to it
    with open('{0}'.format(csv_file), 'w', newline='') as file:
        csv_writer = csv.writer(file)

        # add the table column headers
        # It will look like follows:
        # noc_placement_weight place_cost bb_cost td_cost place_time noc_aggregate_bandwidth_cost noc_latency_cost
        csv_writer.writerow([NOC_PLACEMENT_WEIGHT, PLACE_COST, PLACE_BB_COST, PLACE_TD_COST, PLACE_TIME, NOC_AGGREGATE_BANDWIDTH_COST, NOC_LATENCY_COST])

        # now go through the sorted data for the design and write each run within a single row
        for single_run_data in sorted_data:
            csv_writer.writerow([single_run_data[NOC_PLACEMENT_WEIGHT], single_run_data[PLACE_COST], single_run_data[PLACE_BB_COST], single_run_data[PLACE_TD_COST], single_run_data[PLACE_TIME], single_run_data[NOC_AGGREGATE_BANDWIDTH_COST], single_run_data[NOC_LATENCY_COST]])

        file.close()

def send_notification(test_status, error_message):
    # create the message to send
    message = EmailMessage()
    # set the subject line to match the test status
    if test_status == True:
        message['Subject'] = "NOC TEST PASSED!"
    elif test_status == False:
        message['Subject'] = "NOC TEST FAILED!"
        # if there was en error then the message content will be the error messaged
        message.set_content('TEST FAILED WITH THE FOLLOWING ERROR:\n{0}'.format(error_message))
    
    # set the sender and receiver email address
    message['From'] = Address("****", "****", "****")
    message['To'] = Address("****", "****", "****")

    # now we send the message
    sender_method = smtplib.SMTP(host='****', port=587)
    sender_method.starttls()
    sender_method.login('****', '****')
    sender_method.send_message(message)
    sender_method.quit()

    
    

if __name__ == "__main__":
    
    error_message = ''
    test_status = True

    try:
        # Load the arguments
        args = noc_test_command_line_parser().parse_args(sys.argv[1:])

        # local datastructures that store the design file locations, their corresponding noc traffic flow files and the design name
        design_files = []
        design_names = []
        design_flow_files = []

        find_all_circuit_files(args.tests_folder_location, design_files=design_files, design_names=design_names, design_flow_files=design_flow_files)

        # process each design below
        for single_design, single_design_flows_file, single_design_name in zip(design_files, design_flow_files, design_names):

            # execute vpr place on the current design
            run_vpr(vpr_location=args.vpr_executable, arch_file=args.arch_file, design_file=single_design, design_flows_file=single_design_flows_file, max_noc_weighting=args.max_weighting, noc_weighting_interval=args.interval)

            # lets create a CSV file that will store the 
            write_csv_file(data=design_run_data, csv_file_name=single_design_name)

            # clear the list for the next design
            design_run_data[:] = []
    except Exception as error:
        # the test failed
        error_message = error
        test_status = False  

    send_notification(test_status=test_status, error_message=error)




