import sys
import shutil
import subprocess
import time
import glob
import vtr
from pathlib import Path
from collections import OrderedDict

VTR_STAGE = vtr.make_enum("odin", "abc", 'ace', "vpr", "lec")
vtr_stages = VTR_STAGE.reverse_mapping.values()

def run(architecture_file, circuit_file, 
                 power_tech_file=None,
                 start_stage=VTR_STAGE.odin, end_stage=VTR_STAGE.vpr, 
                 command_runner=vtr.CommandRunner(), 
                 parse_config_file=None,
                 temp_dir="./temp", 
                 verbosity=0,
                 vpr_args=None,
                 abc_args=""):
    """
    Runs the VTR CAD flow to map the specificied circuit_file onto the target architecture_file

    Arguments
    ---------
        architecture_file: Architecture file to target
        circuit_file     : Circuit to implement

        power_tech_file  : Technology power file.  Enables power analysis and runs ace

        temp_dir         : Directory to run in (created if non-existant)
        start_stage      : Stage of the flow to start at
        end_stage        : Stage of the flow to finish at
        command_runner   : A CommandRunner object used to run system commands
        parse_config_file: The configuration file defining how to parse metrics from results
        verbosity        : How much output to produce
        vpr_args         : A dictionary of keywork arguments to pass on to VPR
    """
    if vpr_args == None:
        vpr_args = OrderedDict()

    #
    #Initial setup
    #

    circuit_path = Path(circuit_file)
    architecture_path = Path(architecture_file)
    architecture_file_basename =architecture_path.name
    circuit_file_basename = circuit_path.name

    circuit_name = circuit_path.stem
    circuit_ext = circuit_path.suffixes
    architecture_name = architecture_path.stem
    architecture_ext = architecture_path.suffixes

    vtr.mkdir_p(temp_dir)
    
    #Define useful filenames
    post_odin_netlist = Path(temp_dir)  / (circuit_name + '.odin.blif')
    post_abc_netlist =Path(temp_dir)  / (circuit_name + '.abc.blif')
    post_ace_netlist =Path(temp_dir)  / (circuit_name + ".ace.blif")
    post_ace_activity_file = Path(temp_dir)  / (circuit_name + ".act")
    pre_vpr_netlist = Path(temp_dir)  / (circuit_name + ".pre_vpr.blif")
    post_vpr_netlist = Path(temp_dir)  / "top_post_synthesis.blif" #circuit_name + ".vpr.blif"
    lec_base_netlist = None #Reference netlist for LEC

    if circuit_ext == ".blif":
        #If the user provided a .blif netlist, we use that as the baseline for LEC
        #(ABC can't LEC behavioural verilog)
        lec_base_netlist = circuit_file_basename

    #Copy the circuit and architecture
    circuit_copy = Path(temp_dir)  / circuit_path.name
    architecture_copy = Path(temp_dir)  / architecture_path.name
    shutil.copy(circuit_file, str(circuit_copy))
    shutil.copy(architecture_file, str(architecture_copy))


    #There are multiple potential paths for the netlist to reach a tool
    #We initialize it here to the user specified circuit and let downstream 
    #stages update it
    next_stage_netlist = circuit_copy

    #
    # RTL Elaboration & Synthesis
    #
    if should_run_stage(VTR_STAGE.odin, start_stage, end_stage):
        if circuit_ext != ".blif":
            vtr.print_verbose(1, verbosity, "Running Odin II")

            vtr.odin.run(architecture_copy, next_stage_netlist, 
                     output_netlist=post_odin_netlist, 
                     command_runner=command_runner, 
                     temp_dir=temp_dir)

            next_stage_netlist = post_odin_netlist

            if not lec_base_netlist:
                lec_base_netlist = post_odin_netlist

    #
    # Logic Optimization & Technology Mapping
    #
    if should_run_stage(VTR_STAGE.abc, start_stage, end_stage):
        vtr.print_verbose(1, verbosity, "Running ABC")
        vtr.abc.run(architecture_copy, next_stage_netlist, 
                output_netlist=post_abc_netlist, 
                command_runner=command_runner, 
                temp_dir=temp_dir,
                abc_args=abc_args)

        next_stage_netlist = post_abc_netlist

        if not lec_base_netlist:
            lec_base_netlist = post_abc_netlist


    #
    # Power Activity Estimation
    #
    if power_tech_file:
        #The user provided a tech file, so do power analysis

        if should_run_stage(VTR_STAGE.ace, start_stage, end_stage):
            vtr.print_verbose(1, verbosity, "Running ACE")

            vtr.ace.run(next_stage_netlist, output_netlist=post_ace_netlist, 
                    output_activity_file=post_ace_activity_file, 
                    command_runner=command_runner, 
                    temp_dir=temp_dir)

        #Use ACE's output netlist
        next_stage_netlist = post_ace_netlist

        if not lec_base_netlist:
            lec_base_netlist = post_ace_netlist

        #Enable power analysis in VPR
        vpr_args["power"] = True
        vpr_args["activity_file"] = post_ace_activity_file.name
        vpr_args["tech_properties"] = power_tech_file

    #
    # Pack/Place/Route
    #
    if should_run_stage(VTR_STAGE.vpr, start_stage, end_stage):
        #Copy the input netlist for input to vpr
        shutil.copyfile(str(next_stage_netlist), str(pre_vpr_netlist))

        #Do we need to generate the post-synthesis netlist? (e.g. for LEC)
        if should_run_stage(VTR_STAGE.lec, start_stage, end_stage):
            if "gen_postsynthesis_netlist" not in vpr_args:
                vpr_args["gen_postsynthesis_netlist"] = "on"

        if "route_chan_width" in vpr_args:
            #The User specified a fixed channel width
            vtr.print_verbose(1, verbosity, "Running VPR (at fixed channel width)")
            vtr.vpr.run(architecture_copy, circuit_copy, pre_vpr_netlist, 
                    output_netlist=post_vpr_netlist,
                    command_runner=command_runner, 
                    temp_dir=temp_dir, 
                    vpr_args=vpr_args)
        else:
            #First find minW and then re-route at a relaxed W
            vtr.vpr.run_relax_W(architecture_copy, circuit_copy, pre_vpr_netlist, 
                            output_netlist=post_vpr_netlist,
                            command_runner=command_runner, 
                            temp_dir=temp_dir, 
                            verbosity=verbosity, 
                            vpr_args=vpr_args)

        if not lec_base_netlist:
            lec_base_netlist = pre_vpr_netlist

    #
    # Logical Equivalence Checks (LEC)
    #
    if should_run_stage(VTR_STAGE.lec, start_stage, end_stage):
        vtr.print_verbose(1, verbosity, "Running ABC Logical Equivalence Check")
        vtr.abc.run_lec(lec_base_netlist, post_vpr_netlist, command_runner=command_runner, log_filename="abc.lec.out")

def parse_vtr_flow(temp_dir, parse_config_file=None, metrics_filepath=None, verbosity=1):
    vtr.print_verbose(1, verbosity, "Parsing results")
    vtr.mkdir_p(temp_dir)
    if parse_config_file is None:
        parse_config_file = vtr.find_vtr_file("vtr_benchmarks.txt")

    parse_patterns = vtr.load_parse_patterns(parse_config_file) 

    metrics = OrderedDict()

    #Set defaults
    for parse_pattern in parse_patterns.values():

        if parse_pattern.default_value() != None:
            metrics[parse_pattern.name()] = parse_pattern.default_value()
        else:
            metrics[parse_pattern.name()] = ""

    #Process each pattern
    for parse_pattern in parse_patterns.values():

        #We interpret the parse pattern's filename as a glob pattern
        filepattern = str(Path(temp_dir)  / parse_pattern.filename())
        filepaths = glob.glob(filepattern)

        num_files = len(filepaths)

        if num_files > 1:
            raise vtr.InspectError("File pattern '{}' is ambiguous ({} files matched)".format(parse_pattern.filename()), num_files, filepaths)

        elif num_files == 1:
            filepath = filepaths[0]

            assert Path(filepath).exists

            with open(filepath) as f:
                for line in f:
                    match = parse_pattern.regex().match(line)
                    if match:
                        #Extract the first group value
                        metrics[parse_pattern.name()] = match.groups()[0]
        else:
            #No matching file, skip
            assert num_files == 0

    if metrics_filepath is None:
        metrics_filepath = str(Path(temp_dir)  / "parse_results.txt")

    vtr.write_tab_delimitted_csv(metrics_filepath, [metrics])

    return metrics

def should_run_stage(stage, flow_start_stage, flow_end_stage):
    """
    Returns True if stage falls between flow_start_stage and flow_end_stage
    """
    if flow_start_stage <= stage <= flow_end_stage:
        return True
    return False




