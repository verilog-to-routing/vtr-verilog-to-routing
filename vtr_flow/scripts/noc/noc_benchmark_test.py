#!/usr/bin/env python3

"""
    Module for running tests to verify the NoC placement of vpr
"""

from concurrent.futures import ThreadPoolExecutor
from distutils.log import error
import os
from pathlib import Path
import sys
import argparse
import textwrap
import subprocess
import re as regex
from random import randrange

# pylint: disable=wrong-import-position, import-error
sys.path.insert(0, str(Path(__file__).resolve().parent / "../python_libs"))

from vtr import RawDefaultHelpFormatter

# important extensions
CIRCUIT_FILE_EXT = ".blif"
FLOWS_FILE_EXT = ".flows"
CSV_FILE_EXT = ".csv"
NET_FILE_EXT = ".net"
POST_ROUTING_EXT = ".post_routing"
CONSTRAINTS_FILE_EXT = ".xml"

# informat parsed from the vpr output
PLACE_COST = "place_cost"
PLACE_TIME = "Place Time (s): "
PLACE_BB_COST = "Post Place WL: "
PLACE_CPD = "Post Place CPD (ns): "
NOC_AGGREGATE_BANDWIDTH_COST = "NoC BW Cost: "
NOC_LATENCY_COST = "NoC Latency Cost: "
NOC_LATENCY_CONSTRAINT_COST = "# Of NoC Latency Constraints Met: "
NOC_PLACEMENT_WEIGHT = "noc_placement_weight"
POST_ROUTED_WIRE_LENGTH_SEGMENTS = "Post Route WL (segments): "
POST_ROUTED_FREQ = "Post Route Freq (MHz): "
ROUTE_TIME = "Route Time (s): "

# phrases to identify lines that contain palcement data
PLACEMENT_COST_PHRASE = "Placement cost:"
NOC_PLACEMENT_COST_PHRASE = "NoC Placement Costs."
PLACEMENT_TIME = "# Placement took"
POST_PLACE_CRITICAL_PATH_DELAY_PHRASE = "post-quench CPD"

# phrases to identify lines that contain routing data
POST_ROUTED_WIRE_LENGTH_SEGMENTS_PHRASE = "Total wiring segments used:"
POST_ROUTED_FREQ_PHRASE = "Final critical path delay (least slack):"
ROUTE_TIME_PHRASE = "# Routing took"

# regex match strings to extract placement info
PLACEMENT_COST_REGEX = "Placement cost: (.*), bb_cost: (.*), td_cost: (.*),"
NOC_PLACEMENT_COST_REGEX = (
    "NoC Placement Costs. "
    "noc_aggregate_bandwidth_cost: (.*), noc_latency_cost: (.*), "
    "noc_latency_constraints_cost: (.*),"
)
PLACEMENT_TIME_REGEX = "# Placement took (.*) seconds.*"
POST_PLACE_CRITICAL_PATH_DELAY_REGEX = r"post-quench CPD = (.*) \(ns\)"
POST_ROUTED_WIRE_LENGTH_SEGMENTS_REGEX = ".*Total wiring segments used: (.*), .*"
POST_ROUTED_FREQ_REGEX = r"Final critical path delay \(least slack\): .* ns, Fmax: (.*) MHz.*"
ROUTE_TIME_REGEX = "# Routing took (.*) seconds.*"

MAX_SEED_VAL = 1000000


def noc_test_command_line_parser(prog=None):
    """Parses the arguments of noc_benchmark_test"""

    description = textwrap.dedent(
        """
            Runs VPR placer on one or more NoC benchmark designs.
            Runs VPR multiple times on each circuit and collects
            placement & routing metrics. Each run will use a
            different seed (random initialization) and the averaged values over all runs
            are provided.
        """
    )
    epilog = textwrap.dedent(
        """
            Examples
            --------

                Run the NoC driven placement on a design located at
                ./noc_test_circuits (design should be in .blif format).
                Where we want to run 5 seeds (5 seperate runs)
                using 3 threads (running 3 seperate runs of VPR in parallel).
                For more information on all options run program with '-help'
                parameter.

                    %(prog)s -tests_folder_location ./noc_test_cricuits
                    -flow_file <path_to_flows_file> -arch_file
                    <path_to_arch_file> -number_of_seeds 5 -number_of_threads 3

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
        help="Path to the location of the test designs",
    )

    parser.add_argument(
        "-arch_file",
        default="",
        type=str,
        help="The architecture file the NoC benchamrk designs are placed on",
    )

    parser.add_argument("-vpr_executable", default="", type=str, help="The executable file of VPR")

    parser.add_argument(
        "-device",
        default="EP4SGX110",
        type=str,
        help="The FPGA device name the design will be placed on. Default device is EP4SGX110",
    )

    parser.add_argument(
        "-flow_file",
        default="",
        type=str,
        help="The location of the NoC traffic flow file associated with the design",
    )

    parser.add_argument(
        "-router_lookahead_file",
        default="",
        type=str,
        help="The generated routing lookahead for a architecture",
    )

    parser.add_argument(
        "-placement_delay_lookahead_file",
        default="",
        type=str,
        help="The generated placement delay lookahead for a architecture",
    )

    parser.add_argument(
        "-noc_routing_algorithm",
        default="xy_routing",
        type=str,
        help="Controls the algorithm used by the NoC to route packets",
    )

    parser.add_argument(
        "-noc_placement_weight",
        default=0.6,
        type=float,
        help="Controls the importance of the NoC placement parameters "
        "relative to timing and wirelength of the design",
    )

    parser.add_argument(
        "-noc_latency_constraints_weighting",
        default=1.0,
        type=float,
        help="Controls the importance of meeting all the NoC traffic flow latency constraints",
    )

    parser.add_argument(
        "-noc_latency_weighting",
        default=0.05,
        type=float,
        help="Controls the importance of reducing the latencies of the NoC traffic flows",
    )

    parser.add_argument(
        "-noc_swap_percentage",
        default=0,
        type=float,
        help="Sets the minimum fraction of swaps attempted by the placer that are NoC blocks",
    )

    parser.add_argument(
        "-number_of_seeds",
        default=5,
        type=int,
        help="Number of VPR executions which are then averaged to generate final results",
    )

    parser.add_argument(
        "-number_of_threads", default=1, type=int, help="Number of concurrent VPR runs"
    )

    parser.add_argument(
        "-route",
        action="store_true",
        default="store_false",
        dest="route",
        help="Run VPR routing on the test design",
    )

    parser.add_argument(
        "-route_chan_width", default=300, type=int, help="Channel width used for routing"
    )

    parser.add_argument(
        "-fix_clusters",
        default="",
        type=str,
        help="Constraints file to pass into VPR which will lock down certain \
        clusters in the design",
    )

    return parser


def find_all_circuit_files(
    location_of_designs, flow_file_location, design_files, design_names, design_flow_files
):
    """
    Given a directory, recursively search and store '.blif' circuit files.
    Where each file represents a design that needs to be processed.
    """

    # find all the circuit files located within the provided folder
    for single_design_file_path in Path(location_of_designs).rglob("*{0}".format(CIRCUIT_FILE_EXT)):
        # store the design
        design_files.append(single_design_file_path)

    # find and store the name of the design
    design_names.append(Path(flow_file_location).stem)
    design_flow_files.append(flow_file_location)


def process_vpr_output(vpr_output_file):
    """
    Given an VPR output file log. Go throggh and extract both
    placement and routing information for a given run. Remove the
    log after after.
    """

    open_file = open(vpr_output_file)

    # datastrcuture below stors the palcement data in a disctionary
    placement_data = {}

    # process each line from the VPR output
    for line in open_file:

        # we only care about three lines where the
        # placement costs, noc costs and
        # placement times are located
        # and post route info
        # so identify those lines below
        if PLACEMENT_COST_PHRASE in line:
            process_placement_costs(placement_data, line)

        if POST_PLACE_CRITICAL_PATH_DELAY_PHRASE in line:
            process_placement_cpd(placement_data, line)

        if NOC_PLACEMENT_COST_PHRASE in line:
            process_noc_placement_costs(placement_data, line)

        if PLACEMENT_TIME in line:
            process_placement_time(placement_data, line)

        if POST_ROUTED_WIRE_LENGTH_SEGMENTS_PHRASE in line:
            process_post_route_wirelength(placement_data, line)

        if POST_ROUTED_FREQ_PHRASE in line:
            process_post_route_freq(placement_data, line)

        if ROUTE_TIME_PHRASE in line:
            process_route_time(placement_data, line)

    # close and delete the output file
    open_file.close()
    os.remove(vpr_output_file)

    return placement_data


def process_placement_costs(placement_data, line_with_data):
    """
    Given a string which contains palcement data. Extract the total
    placement cost and wirelength cost.
    """

    # extract the placement costs from the line where they are located
    found_placement_metrics = regex.search(PLACEMENT_COST_REGEX, line_with_data)

    # quick check that the regex worked properly
    if (found_placement_metrics is None) or (found_placement_metrics.lastindex != 3):
        raise Exception("Placement cost not written out correctly")

    # we know the order of the different placement costs
    # as they are found within the extracted metric list above.
    #
    # 1st element is the overall placement cost, second element is the
    # placement bb cost and the third element is the placement td cost.
    #
    # Covert them to floats and store them (we don't care about the td cost so # ignore it)
    placement_data[PLACE_COST] = float(found_placement_metrics.group(1))
    placement_data[PLACE_BB_COST] = float(found_placement_metrics.group(2))


def process_placement_cpd(placement_data, line_with_data):
    """
    Given a string which contains the CPD of a placed design, extract
    it from the string.
    """

    # extract the placement time from the line where it is located
    found_placement_metrics = regex.search(POST_PLACE_CRITICAL_PATH_DELAY_REGEX, line_with_data)

    # quick check that the regex worked properly
    if (found_placement_metrics is None) or (found_placement_metrics.lastindex != 1):
        raise Exception("Placement cpd not written out correctly")

    # there should be only one element, since we are only grabbing the placement critical path delay
    placement_data[PLACE_CPD] = float(found_placement_metrics.group(1))


def process_noc_placement_costs(placement_data, line_with_data):
    """
    Given a string which contains NoC placement cost values, extract them
    from the string. The following costs are extarcted:
    - NoC aggregate bandwidth cost
    - NoC latency cost
    - Number of latency constraints met (NoC latency constraints cost)
    """

    # extract the noc placement costs from the line where they are located
    found_placement_metrics = regex.search(NOC_PLACEMENT_COST_REGEX, line_with_data)

    # quick check that the regex worked properly
    if (found_placement_metrics is None) or (found_placement_metrics.lastindex != 3):
        raise Exception("Placement noc cost not written out correctly")

    # we know the order of the different noc placement costs as they are found
    # within the extracted metric list above.
    #
    # 1st element is the noc aggregate bandwidth cost and the second element is
    # the noc latency cost and the third element is the number of latency
    # constraints met.
    placement_data[NOC_AGGREGATE_BANDWIDTH_COST] = float(found_placement_metrics.group(1))
    placement_data[NOC_LATENCY_COST] = float(found_placement_metrics.group(2))
    placement_data[NOC_LATENCY_CONSTRAINT_COST] = int(found_placement_metrics.group(3))


def process_placement_time(placement_data, line_with_data):
    """
    Given a string which stores the placement time of a VPR run,
    extract it.
    """

    # extract the placement time from the line where it is located
    found_placement_metrics = regex.search(PLACEMENT_TIME_REGEX, line_with_data)

    # quick check that the regex worked properly
    if (found_placement_metrics is None) or (found_placement_metrics.lastindex != 1):
        raise Exception("Placement time not written out correctly")

    # there should be only one element, since we are only grabbing the placement time
    placement_data[PLACE_TIME] = float(found_placement_metrics.group(1))


def process_post_route_wirelength(placement_data, line_with_data):
    """
    Given a string which stores the wirelenth after routing,
    extract it.
    """

    # extract wirelength from the line it is located in
    found_routing_metrics = regex.search(POST_ROUTED_WIRE_LENGTH_SEGMENTS_REGEX, line_with_data)

    # check if regex worked correctly
    if (found_routing_metrics is None) or (found_routing_metrics.lastindex != 1):
        raise Exception("Routed wirelength not written out correctly")

    # there should be only one element, since we are only grabbing the routeed wirelength
    placement_data[POST_ROUTED_WIRE_LENGTH_SEGMENTS] = float(found_routing_metrics.group(1))


def process_post_route_freq(placement_data, line_with_data):
    """
    Given a string which stores the frequency after routing,
    extract it.
    """

    # extract freq from the line it is located in
    found_routing_metrics = regex.search(POST_ROUTED_FREQ_REGEX, line_with_data)

    # check if regex worked correctly
    if (found_routing_metrics is None) or (found_routing_metrics.lastindex != 1):
        raise Exception("Routed frequency not written out correctly")

    # there should be only one element, since we are only grabbing the routed frequency
    placement_data[POST_ROUTED_FREQ] = float(found_routing_metrics.group(1))


def process_route_time(placement_data, line_with_data):
    """
    Given a string which stores the total route time,
    extract it.
    """

    # extract route time from the line it is located in
    found_routing_metrics = regex.search(ROUTE_TIME_REGEX, line_with_data)

    # check if regex worked correctly
    if (found_routing_metrics is None) or (found_routing_metrics.lastindex != 1):
        raise Exception("Routing time not written out correctly")

    # there should be only one element, since we are only grabbing the route time
    placement_data[ROUTE_TIME] = float(found_routing_metrics.group(1))


def check_for_constraints_file(design_file):
    """
    Check if a given design has an accompanying constraints file which
    needs to be included within the VPR run.
    """

    # build the constraint file path
    constraints_file = os.path.splitext(design_file)[0] + CONSTRAINTS_FILE_EXT

    # now check if this file exists
    file_to_check = Path(constraints_file)

    if not file_to_check.is_file():
        # file doesn't exist. so set the file path to an empty string
        constraints_file = ""

    return constraints_file


def gen_vpr_run_command(design_file, design_flows_file, user_args):
    """
    Generate a seperate VPR run commands each with a unique placement
    seed value. The number of commands generated is equal to the number
    of seeds the user requested to run.
    For each run we generate seperate '.net' files. This was needed
    since a single net file caused failures when multiple concurrent
    VPR runs tried accessing the file during placement.
    """

    # stores a list of VPR commands to execute for each seed
    vpr_run_commands = []

    # Generate a new command for the total number of seeds
    # pylint: disable=unused-variable
    for seed_run in range(user_args.number_of_seeds):
        # generate a random seed val for the current run
        seed_val = randrange(1, MAX_SEED_VAL)

        # stores the vpr net file names
        vpr_net_file = "{0}.{1}{2}".format(design_file, seed_val, NET_FILE_EXT)

        single_seed_vpr_command = [
            user_args.vpr_executable,
            user_args.arch_file,
            str(design_file),
            "--noc",
            "on",
            "--noc_flows_file",
            design_flows_file,
            "--noc_routing_algorithm",
            user_args.noc_routing_algorithm,
            "--device",
            user_args.device,
            "--noc_latency_weighting",
            str(user_args.noc_latency_weighting),
            "--pack",
            "--place",
            "--net_file",
            vpr_net_file,
            "--seed",
            str(seed_val),
            "--noc_latency_constraints_weighting",
            str(user_args.noc_latency_constraints_weighting),
            "--noc_placement_weighting",
            str(user_args.noc_placement_weight),
            "--noc_swap_percentage",
            str(user_args.noc_swap_percentage),
        ]

        # now add variable parameters based on input
        # check if there is also a constraints file we need to worry about
        constraints_file = check_for_constraints_file(design_file=design_file)
        if constraints_file != "":
            # this means there is a constraints file we need to include in the
            # command line arguments
            single_seed_vpr_command.append("--read_vpr_constraints")
            single_seed_vpr_command.append(constraints_file)

        if user_args.router_lookahead_file != "":
            single_seed_vpr_command.append("--read_router_lookahead")
            single_seed_vpr_command.append(user_args.router_lookahead_file)

        if user_args.placement_delay_lookahead_file != "":
            single_seed_vpr_command.append("--read_placement_delay_lookup")
            single_seed_vpr_command.append(user_args.placement_delay_lookahead_file)

        if user_args.fix_clusters != "":
            single_seed_vpr_command.append("--fix_clusters")
            single_seed_vpr_command.append(user_args.fix_clusters)

        if user_args.route is True:
            # user wanted to route design so add params to run router
            single_seed_vpr_command.append("--route")
            single_seed_vpr_command.append("--route_chan_width")
            single_seed_vpr_command.append(str(user_args.route_chan_width))

        # now add the newly created command to the list of all commands to execute
        vpr_run_commands.append(single_seed_vpr_command)

    return vpr_run_commands


def run_vpr_command_and_store_output(vpr_output_file, vpr_run_command):
    """
    Execute a single VPR run using 1 thread and store the output to a
    newly created file.
    """

    # create the file that will store the VPR output
    vpr_output = open(vpr_output_file, "w")
    # run VPR. Will timeout after 10 hours (should be good for any design)
    subprocess.run(vpr_run_command, check=True, stdout=vpr_output, timeout=36000)
    # close the output file
    vpr_output.close()


def process_vpr_runs(run_args, num_of_seeds, route):
    """
    Goes through the log files for every VPR run for a single design
    and extracts relevant placement & routing metrics which is
    then stored internally. The placement metrics are averaged over
    all VPR runs. The ".net" and output log files for
    each VPR run is then removed.
    """

    # stores the average of all results for the current test
    vpr_average_place_data = {}
    vpr_average_place_data[PLACE_BB_COST] = 0.0
    vpr_average_place_data[PLACE_COST] = 0.0
    vpr_average_place_data[PLACE_TIME] = 0.0
    vpr_average_place_data[PLACE_CPD] = 0.0
    vpr_average_place_data[NOC_AGGREGATE_BANDWIDTH_COST] = 0.0
    vpr_average_place_data[NOC_LATENCY_COST] = 0.0
    vpr_average_place_data[NOC_LATENCY_CONSTRAINT_COST] = 0.0

    # add route metrics
    vpr_average_place_data[POST_ROUTED_WIRE_LENGTH_SEGMENTS] = 0.0
    vpr_average_place_data[POST_ROUTED_FREQ] = 0.0
    vpr_average_place_data[ROUTE_TIME] = 0.0

    # weight used for latency component of cost
    latency_weight = float(run_args[0][1][12])

    for single_run_args in run_args:

        # get the placement metrics for the current run
        curr_vpr_place_data = process_vpr_output(vpr_output_file=single_run_args[0])

        # now accumulate the results of the current seed run
        vpr_average_place_data[PLACE_BB_COST] = (
            vpr_average_place_data[PLACE_BB_COST] + curr_vpr_place_data[PLACE_BB_COST]
        )
        vpr_average_place_data[PLACE_COST] = (
            vpr_average_place_data[PLACE_COST] + curr_vpr_place_data[PLACE_COST]
        )
        vpr_average_place_data[PLACE_TIME] = (
            vpr_average_place_data[PLACE_TIME] + curr_vpr_place_data[PLACE_TIME]
        )
        vpr_average_place_data[PLACE_CPD] = (
            vpr_average_place_data[PLACE_CPD] + curr_vpr_place_data[PLACE_CPD]
        )
        vpr_average_place_data[NOC_AGGREGATE_BANDWIDTH_COST] = (
            vpr_average_place_data[NOC_AGGREGATE_BANDWIDTH_COST]
            + curr_vpr_place_data[NOC_AGGREGATE_BANDWIDTH_COST]
        )
        vpr_average_place_data[NOC_LATENCY_COST] = (
            vpr_average_place_data[NOC_LATENCY_COST] + curr_vpr_place_data[NOC_LATENCY_COST]
        )
        vpr_average_place_data[NOC_LATENCY_CONSTRAINT_COST] = (
            vpr_average_place_data[NOC_LATENCY_CONSTRAINT_COST]
            + curr_vpr_place_data[NOC_LATENCY_CONSTRAINT_COST]
        )

        if route is True:
            vpr_average_place_data[POST_ROUTED_WIRE_LENGTH_SEGMENTS] = (
                vpr_average_place_data[POST_ROUTED_WIRE_LENGTH_SEGMENTS]
                + curr_vpr_place_data[POST_ROUTED_WIRE_LENGTH_SEGMENTS]
            )
            vpr_average_place_data[POST_ROUTED_FREQ] = (
                vpr_average_place_data[POST_ROUTED_FREQ] + curr_vpr_place_data[POST_ROUTED_FREQ]
            )
            vpr_average_place_data[ROUTE_TIME] = (
                vpr_average_place_data[ROUTE_TIME] + curr_vpr_place_data[ROUTE_TIME]
            )

        # delete the net file associated with the current run
        os.remove(single_run_args[1][16])
        # delete net file after routing for current run
        os.remove(single_run_args[1][16] + POST_ROUTING_EXT)

    # get the average palacement results after all the runs
    vpr_average_place_data = {
        place_param: value / num_of_seeds for place_param, value in vpr_average_place_data.items()
    }

    # need to divide the NoC latency cost by the weighting to conver it to
    # physical latency
    vpr_average_place_data[NOC_LATENCY_COST] = (
        vpr_average_place_data[NOC_LATENCY_COST] / latency_weight
    )

    return vpr_average_place_data


def print_results(parsed_data, design_file, user_args):
    """
    Outputs collected metrics from VPR runs into a file.
    """
    results_file_name = (os.path.splitext(user_args.flow_file))[-2]
    results_file_name = (results_file_name.split("/"))[-1]
    results_file_name = os.path.join(os.getcwd(), results_file_name + ".txt")
    results_file = open(results_file_name, "w+")

    # write out placement info individually in seperate lines
    results_file.write("Design File: {0}\n".format(design_file))
    results_file.write("Flows File: {0}\n".format(user_args.flow_file))

    results_file.write("------------ Place & Route Info ------------\n")
    for metric, value in parsed_data.items():
        results_file.write("{0}: {1}\n".format(metric, value))

    results_file.close()


def execute_vpr_and_process_output(vpr_command_list, num_of_seeds, num_of_threads, route):
    """
    Given a number of VPR run commands, execute them over a number
    of available threads. A unique file name is created for each VPR
    run to store its output.
    Once all runs have executed, then process their output logs and store
    placement metrics.
    """

    # stores arguments which will be used to run vpr
    run_args = []

    for single_vpr_command in vpr_command_list:

        # generate VPR output file_name
        # the constants represent the positions of the variabels in the command list
        design_file_name = single_vpr_command[2]
        seed_val = single_vpr_command[18]
        vpr_out_file = "{0}.{1}.vpr.out".format(design_file_name, seed_val)

        # group the output file and VPR command together.
        # These two components are arguments to the function which executes the
        # vpr command
        single_run_args = (vpr_out_file, single_vpr_command)

        run_args.append(single_run_args)

    # create the thread pool that will run vpr in parallel
    vpr_executor = ThreadPoolExecutor(max_workers=num_of_threads)

    # run all instances of vpr
    vpr_executor.map(lambda vpr_args: run_vpr_command_and_store_output(*vpr_args), run_args)

    # wait for all the vpr runs to finish
    vpr_executor.shutdown(wait=True)

    # process the output and return result
    return process_vpr_runs(run_args=run_args, num_of_seeds=num_of_seeds, route=route)


if __name__ == "__main__":

    try:
        # Load the arguments
        args = noc_test_command_line_parser().parse_args(sys.argv[1:])

        """
         local data structures that store the design file locations, their
         corresponding noc traffic flow files and the design name
        """
        design_files_in_dir = []
        design_names_in_dir = []
        design_flow_files_in_dir = []

        find_all_circuit_files(
            location_of_designs=args.tests_folder_location,
            flow_file_location=args.flow_file,
            design_files=design_files_in_dir,
            design_names=design_names_in_dir,
            design_flow_files=design_flow_files_in_dir,
        )

        # process each design below
        for single_design, single_design_flows_file, single_design_name in zip(
            design_files_in_dir, design_flow_files_in_dir, design_names_in_dir
        ):

            # generate all the vpr commands
            vpr_commands = gen_vpr_run_command(
                design_file=single_design,
                design_flows_file=single_design_flows_file,
                user_args=args,
            )

            # run VPR
            vpr_placement_results = execute_vpr_and_process_output(
                vpr_command_list=vpr_commands,
                num_of_seeds=args.number_of_seeds,
                num_of_threads=args.number_of_threads,
                route=args.route,
            )

            print_results(
                parsed_data=vpr_placement_results, design_file=single_design, user_args=args
            )

    # pylint: disable=broad-except
    except Exception as error:
        # the test failed so let user know why
        print("TEST FAILED: " + str(error))
