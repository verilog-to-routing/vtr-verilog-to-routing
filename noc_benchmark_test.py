#!/usr/bin/env python3

"""
    Module for running tests to verify the NoC placement of vpr
"""

from concurrent.futures import ThreadPoolExecutor, wait
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
import getpass
from random import randrange
import threading

sys.path.insert(0, str(Path(__file__).resolve().parent / "vtr_flow/scripts/python_libs"))
sys.path.insert(0, str(Path(__file__).resolve().parent / "vtr_flow/scripts"))

from vtr import RawDefaultHelpFormatter, paths, vpr

# important extensions
CIRCUIT_FILE_EXT = ".blif"
FLOWS_FILE_EXT = ".flows"
CSV_FILE_EXT = ".csv"
NET_FILE_EXT = ".net"
CONSTRAINTS_FILE_EXT = ".xml"

# informat parsed from the vpr output
PLACE_COST = "place_cost"
PLACE_TIME = "place_time"
PLACE_BB_COST = "bb_cost"
PLACE_CPD = "post_place_cpd"
NOC_AGGREGATE_BANDWIDTH_COST = "noc_aggregate_bandwidth_cost"
NOC_LATENCY_COST = "noc_latency_cost"
NOC_PLACEMENT_WEIGHT = "noc_placement_weight"

#phrases to identify lines that contain palcement data
PLACEMENT_COST_PHRASE = "Placement cost:"
NOC_PLACEMENT_COST_PHRASE = "NoC Placement Costs."
PLACEMENT_TIME = "# Placement took"
POST_PLACE_CRITICAL_PATH_DELAY_PHRASE = "post-quench CPD"

# regex match strings to extract placement info
PLACEMENT_COST_REGEX = 'Placement cost: (.*), bb_cost: (.*), td_cost: (.*),'
NOC_PLACEMENT_COST_REGEX = 'NoC Placement Costs. noc_aggregate_bandwidth_cost: (.*), noc_latency_cost: (.*),'
PLACEMENT_TIME_REGEX = '# Placement took (.*) seconds.*'
POST_PLACE_CRITICAL_PATH_DELAY_REGEX = 'post-quench CPD = (.*) \(ns\)'

NUM_OF_SEED_RUNS = 5
MAX_SEED_VAL = 10000

# global synhronization datastructures
main_manager = Manager()
design_run_data = main_manager.list()
single_weight_multiple_runs = main_manager.list()


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

        if POST_PLACE_CRITICAL_PATH_DELAY_PHRASE in line:
            process_placement_cpd(placement_data, line)

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
    if (found_placement_metrics is None) or (found_placement_metrics.lastindex != 3):
        raise Exception("Placement cost not written out correctly")

    # we know the order of the different placement costs as they are found within the extracted metric list above
    # 1st element is the overall placement cost, second element is the placement bb cost and the third element is the placement td cost
    # covert them to floats and store them (we don't care about the td cost so ignore it)
    placement_data[PLACE_COST] = float(found_placement_metrics.group(1))
    placement_data[PLACE_BB_COST] = float(found_placement_metrics.group(2))
    
    return

def process_placement_cpd(placement_data, line_with_data):

    # extract the placement time from the line where it is located
    found_placement_metrics = regex.search(POST_PLACE_CRITICAL_PATH_DELAY_REGEX, line_with_data)

    # quick check that the regex worked properly
    if  (found_placement_metrics is None) or (found_placement_metrics.lastindex != 1):
        raise Exception("Placement cpd not written out correctly")

    # there should be only one element, since we are only grabbing the placement critical path delay
    placement_data[PLACE_CPD] = float(found_placement_metrics.group(1))
    
    return

def process_noc_placement_costs(placement_data, line_with_data):

    # extract the noc placement costs from the line where they are located
    found_placement_metrics = regex.search(NOC_PLACEMENT_COST_REGEX, line_with_data)

    # quick check that the regex worked properly
    if (found_placement_metrics is None) or (found_placement_metrics.lastindex != 2):
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

def check_for_constraints_file(design_file):
    
    # build the constraint file path
    constraints_file = os.path.splitext(design_file)[0] + CONSTRAINTS_FILE_EXT

    # now check if this file exists
    file_to_check = Path(constraints_file)

    if not file_to_check.is_file():
        # file doesn't exist. so set the file path to an empty string
        constraints_file = ''

    return constraints_file

def run_vpr_command_and_store_output(vpr_location, arch_file, design_file, design_flows_file, routing_algorithm, device, noc_placement_weight, vpr_out_file, vpr_net_file, seed_val):
    # create the file that will store the VPR output
    vpr_output = open(vpr_out_file, 'w')

    vpr_command = [vpr_location, arch_file, str(design_file), '--noc', 'on', '--noc_flows_file', design_flows_file, '--noc_routing_algorithm', routing_algorithm, '--device', device, '--noc_placement_weighting', str(noc_placement_weight), '--pack', '--place', '--net_file', vpr_net_file, '--seed', str(seed_val)]

    # check if there is also a constraints file we need to worry about
    constraints_file = check_for_constraints_file(design_file=design_file)

    if constraints_file != '':
        # this means there is a constraints file we need to include in the command line arguments
        vpr_command.append('--read_vpr_constraints')
        vpr_command.append(constraints_file)

    # run vpr. Will timeout after 2 hours (maybe we need more for larger designs
    result = subprocess.run(vpr_command,check=True, stdout=vpr_output, timeout=7200)

    #close the output file
    vpr_output.close()


def execute_vpr_and_process_output(vpr_location, arch_file, design_file, design_flows_file, routing_algorithm, device, noc_placement_weight, vpr_out_file, vpr_net_file):

    # stores a list of arguments that are needed for the vpr runs
    run_args = []

    # create the test args for all the seed runs
    for seed_run in range(NUM_OF_SEED_RUNS):
        # generate a random seed val
        seed_val = randrange(1, MAX_SEED_VAL)

        # generate the new net and output file names
        new_vpr_out_file = os.path.splitext(vpr_out_file)[0]
        new_vpr_out_file = os.path.splitext(new_vpr_out_file)[0]
        new_vpr_out_file = '{0}.{1}.vpr.out'.format(new_vpr_out_file, seed_val)

        new_vpr_net_file = os.path.splitext(vpr_net_file)[0]
        new_vpr_net_file = '{0}.{1}{2}'.format(new_vpr_net_file, seed_val, NET_FILE_EXT)

        run_args.append((vpr_location, arch_file, design_file, design_flows_file, routing_algorithm, device, noc_placement_weight, new_vpr_out_file, new_vpr_net_file, seed_val))

    # create the thread pool that will run vpr in parallel
    vpr_executor = ThreadPoolExecutor(max_workers=1)

    # run all instances of vpr
    vpr_run_result = vpr_executor.map(lambda vpr_args: run_vpr_command_and_store_output(*vpr_args), run_args)
    # wait for all the vpr runs to finish
    vpr_executor.shutdown(wait=True)

    # stores the average of all results for the current test
    vpr_average_place_data = {}
    vpr_average_place_data[PLACE_BB_COST] = 0.0
    vpr_average_place_data[PLACE_COST] = 0.0
    vpr_average_place_data[PLACE_TIME] = 0.0
    vpr_average_place_data[PLACE_CPD] = 0.0
    vpr_average_place_data[NOC_AGGREGATE_BANDWIDTH_COST] = 0.0
    vpr_average_place_data[NOC_LATENCY_COST] = 0.0

    for single_run_args in run_args:

        # get the placement metrics for the current run
        curr_vpr_place_data = process_vpr_output(vpr_output_file=single_run_args[7])

        # now accumulate the results of the current seed run
        vpr_average_place_data[PLACE_BB_COST] = vpr_average_place_data[PLACE_BB_COST] + curr_vpr_place_data[PLACE_BB_COST]
        vpr_average_place_data[PLACE_COST] = vpr_average_place_data[PLACE_COST] + curr_vpr_place_data[PLACE_COST]
        vpr_average_place_data[PLACE_TIME] = vpr_average_place_data[PLACE_TIME] + curr_vpr_place_data[PLACE_TIME]
        vpr_average_place_data[PLACE_CPD] = vpr_average_place_data[PLACE_CPD] + curr_vpr_place_data[PLACE_CPD]
        vpr_average_place_data[NOC_AGGREGATE_BANDWIDTH_COST] = vpr_average_place_data[NOC_AGGREGATE_BANDWIDTH_COST] + curr_vpr_place_data[NOC_AGGREGATE_BANDWIDTH_COST]
        vpr_average_place_data[NOC_LATENCY_COST] = vpr_average_place_data[NOC_LATENCY_COST] + curr_vpr_place_data[NOC_LATENCY_COST]

        # delete the net file associated with the current run
        os.remove(single_run_args[8])
    
    # get the average palacement results after all the runs
    vpr_average_place_data = {place_param: value / NUM_OF_SEED_RUNS for place_param, value in vpr_average_place_data.items()}
    
    # indicate what the current noc weighting is 
    vpr_average_place_data[NOC_PLACEMENT_WEIGHT] = noc_placement_weight

    # store the current run data
    design_run_data.append(vpr_average_place_data)


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
    execute_pool = Pool(2)
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
        csv_writer.writerow([NOC_PLACEMENT_WEIGHT, PLACE_COST, PLACE_BB_COST, PLACE_CPD, PLACE_TIME, NOC_AGGREGATE_BANDWIDTH_COST, NOC_LATENCY_COST])

        # now go through the sorted data for the design and write each run within a single row
        for single_run_data in sorted_data:
            csv_writer.writerow([single_run_data[NOC_PLACEMENT_WEIGHT], single_run_data[PLACE_COST], single_run_data[PLACE_BB_COST], single_run_data[PLACE_CPD], single_run_data[PLACE_TIME], single_run_data[NOC_AGGREGATE_BANDWIDTH_COST], single_run_data[NOC_LATENCY_COST]])

        file.close()

def send_notification(test_status, error_message, email_related_info):
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
    message['From'] = Address(email_related_info['email_display_name'], email_related_info['email_user_name'], email_related_info['domain'])
    message['To'] = Address(email_related_info['email_display_name'], email_related_info['email_user_name'], email_related_info['domain'])

    # now we send the message
    email_related_info['sender_method'].send_message(message)
    email_related_info['sender_method'].quit()

def get_user_email_info_and_authenticate():

    email_related_info = {}

    # the user email address
    email_related_info['user_email'] = input("Email Address: ")
    email_related_info['email_display_name'] = input("Display Name (your full name): ")
    email_related_info['email_user_name'] = input("Email User Name: ")
    email_related_info['domain'] = input("Domain: ")
    email_related_info['email_password'] = getpass.getpass("Email Password: ")

    # now login to the smtp server (we only support outlook right now)
    try:
        email_related_info['sender_method'] = smtplib.SMTP(host='smtp.office365.com', port=587)
        email_related_info['sender_method'].starttls()
        email_related_info['sender_method'].login(email_related_info['user_email'], email_related_info['email_password'])
    except Exception as email_setup_error:
        print("Email setup failed with error:\n{0}".format(email_setup_error))
        exit()
    
    email_related_info['sender_method'].quit()

    return email_related_info


if __name__ == "__main__":
    
    error_message = ''
    test_status = True

    # get user information and authenticate them
    email_setup = get_user_email_info_and_authenticate()

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
    
    # need to reinitiate a connection to the server
    email_setup['sender_method'] = smtplib.SMTP(host='smtp.office365.com', port=587)
    email_setup['sender_method'].starttls()
    email_setup['sender_method'].login(email_setup['user_email'], email_setup['email_password'])
    send_notification(test_status=test_status, error_message=error_message, email_related_info=email_setup)
    



