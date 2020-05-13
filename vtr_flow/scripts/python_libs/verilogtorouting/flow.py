import sys
import shutil
import subprocess
import time
import glob
from pathlib import Path
from collections import OrderedDict

from util import make_enum, print_verbose, mkdir_p, find_vtr_file, file_replace, relax_W, CommandRunner, write_tab_delimitted_csv
from inspect import determine_memory_addr_width, determine_lut_size, determine_min_W, load_parse_patterns
from error import *

VTR_STAGE = make_enum("odin", "abc", 'ace', "vpr", "lec")
vtr_stages = VTR_STAGE.reverse_mapping.values()

def run_vtr_flow(architecture_file, circuit_file, 
                 power_tech_file=None,
                 start_stage=VTR_STAGE.odin, end_stage=VTR_STAGE.vpr, 
                 command_runner=CommandRunner(), 
                 parse_config_file=None,
                 temp_dir="./temp", 
                 verbosity=0,
                 vpr_args=None,
                 flow_type=2,
                 use_old_latches_restoration_script=1):
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

    mkdir_p(temp_dir)

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
            print_verbose(1, verbosity, "Running Odin II")

            run_odin(architecture_copy, next_stage_netlist, 
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
        print_verbose(1, verbosity, "Running ABC")
        run_abc(architecture_copy, next_stage_netlist, 
                output_netlist=post_abc_netlist, 
                command_runner=command_runner, 
                temp_dir=temp_dir,
                flow_type=flow_type)

        next_stage_netlist = post_abc_netlist

        if not lec_base_netlist:
            lec_base_netlist = post_abc_netlist


    #
    # Power Activity Estimation
    #
    if power_tech_file:
        #The user provided a tech file, so do power analysis

        if should_run_stage(VTR_STAGE.ace, start_stage, end_stage):
            print_verbose(1, verbosity, "Running ACE")

            run_ace(next_stage_netlist, output_netlist=post_ace_netlist, 
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
            print_verbose(1, verbosity, "Running VPR (at fixed channel width)")
            run_vpr(architecture_copy, circuit_copy, pre_vpr_netlist, 
                    output_netlist=post_vpr_netlist,
                    command_runner=command_runner, 
                    temp_dir=temp_dir, 
                    vpr_args=vpr_args)
        else:
            #First find minW and then re-route at a relaxed W
            run_vpr_relax_W(architecture_copy, circuit_copy, pre_vpr_netlist, 
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
        print_verbose(1, verbosity, "Running ABC Logical Equivalence Check")
        run_abc_lec(lec_base_netlist, post_vpr_netlist, command_runner=command_runner, log_filename="abc.lec.out")

def parse_vtr_flow(temp_dir, parse_config_file=None, metrics_filepath=None, verbosity=1):
    print_verbose(1, verbosity, "Parsing results")

    if parse_config_file is None:
        parse_config_file = find_vtr_file("vtr_benchmarks.txt")

    parse_patterns = load_parse_patterns(parse_config_file) 

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
            raise InspectError("File pattern '{}' is ambiguous ({} files matched)".format(parse_pattern.filename()), num_files, filepaths)

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

    write_tab_delimitted_csv(metrics_filepath, [metrics])

    return metrics

def run_odin(architecture_file, circuit_file, 
             output_netlist, 
             command_runner, 
             temp_dir=".", 
             log_filename="odin.out", 
             odin_exec=None, 
             odin_config=None, 
             min_hard_mult_size=3, 
             min_hard_adder_size=1):
    mkdir_p(temp_dir)

    if odin_exec == None:
        odin_exec = find_vtr_file('odin_II', is_executable=True)

    if odin_config == None:
        odin_base_config = find_vtr_file('basic_odin_config_split.xml')

        #Copy the config file
        odin_config = "odin_config.xml"
        odin_config_full_path = str(Path(temp_dir)  / odin_config)
        shutil.copyfile(odin_base_config, odin_config_full_path)

        #Update the config file
        file_replace(odin_config_full_path, {
                                            "XXX": circuit_file.name,
                                            "YYY": architecture_file.name,
                                            "ZZZ": output_netlist.name,
                                            "PPP": determine_memory_addr_width(str(architecture_file)),
                                            "MMM": min_hard_mult_size,
                                            "AAA": min_hard_adder_size,
                                        })

    cmd = [odin_exec, "-c", odin_config, "--adder_type", "default", "-U0"]

    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1)

def populate_clock_list(circuit_file,blackbox_latches_script,clk_list,command_runner,temp_dir,log_filename):
    clk_list_path = Path(temp_dir) / "report_clk.out"
    cmd = [blackbox_latches_script, "--input", circuit_file.name,"--output_list", clk_list_path.name]
    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1)
    with clk_list_path.open('r') as f:
         clk_list.append(f.readline().strip('\n'))

def run_abc(architecture_file, circuit_file, output_netlist, command_runner, temp_dir=".", log_filename="abc.opt_techmap.out", abc_exec=None, abc_script=None, abc_rc=None,use_old_abc_script = False,flow_type=2,use_old_latches_restoration_script=1):
    mkdir_p(temp_dir)
    blackbox_latches_script = find_vtr_file("blackbox_latches.pl")
    clk_list = []
    clk_log_file = "report_clk_out.out"
    if(flow_type):
        populate_clock_list(circuit_file,blackbox_latches_script,clk_list,command_runner,temp_dir,log_filename)

    if abc_exec == None:
        abc_exec = find_vtr_file('abc', is_executable=True)
    
    if abc_rc == None:
        abc_rc = Path(abc_exec).parent / 'abc.rc'

    shutil.copyfile(str(abc_rc), str(Path(temp_dir) / 'abc.rc'))
    lut_size = determine_lut_size(str(architecture_file))
    iterations=len(clk_list)
    
    if(iterations==0 or flow_type != 2):
        iterations=1

    for i in range(0, iterations):
        pre_abc_blif= Path(temp_dir) / (str(i)+"_" + circuit_file.name)
        post_abc_blif = Path(temp_dir) / (str(i)+"_"+output_netlist.name)
        post_abc_raw_blif = Path(temp_dir) / (str(i)+"_raw_"+output_netlist.name)
        if(flow_type==3):
            cmd = [blackbox_latches_script, "--input", circuit_file.name,"--output",pre_abc_blif.name]
            command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=str(i)+"_blackboxing_latch.out", indent_depth=1)
        
        elif(len(clk_list)>i):
            cmd = [blackbox_latches_script,"--clk_list", clk_list[i], "--input", circuit_file.name,"--output",pre_abc_blif.name]
            command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=str(i)+"_blackboxing_latch.out", indent_depth=1)
        else:
            pre_abc_blif = circuit_file
        
        if abc_script == None:
           
            abc_script = ['read {pre_abc_blif}'.format(pre_abc_blif=pre_abc_blif.name),
                        'time',
                        'print_stats',
                        'print_latch',
                        'time',
                        'print_lut',
                        'time',
                        'strash',
                        'ifraig -v',
                        'scorr -v',
                        'dc2 -v',
                        'dch -f',
                        'if -K {lut_size} -v'.format(lut_size=lut_size),
                        'mfs2 -v',
                        'print_stats',
                        'time',
                        'write_hie {pre_abc_blif} {post_abc_raw_blif}'.format(pre_abc_blif=pre_abc_blif.name, post_abc_raw_blif=post_abc_raw_blif.name),
                        'time;']

            if(use_old_abc_script):
                abc_script = ['read {pre_abc_blif}'.format(pre_abc_blif=pre_abc_blif.name),
                        'time',
                        'resyn', 
                        'resyn2', 
                        'if -K {lut_size}'.format(lut_size=lut_size),
                        'time',
                        'scleanup',
                        'write_hie {pre_abc_blif} {post_abc_raw_blif}'.format(pre_abc_blif=pre_abc_blif.name, post_abc_raw_blif=post_abc_raw_blif.name),
                        'print_stats']

            abc_script = "; ".join(abc_script)

        cmd = [abc_exec, '-c', abc_script]

        command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1)
        
        if(flow_type != 3 and len(clk_list)>i):
            cmd = [blackbox_latches_script,"--restore", clk_list[i], "--input", post_abc_raw_blif.name,"--output",post_abc_blif.name]
            command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename="restore_latch" + str(i) + ".out", indent_depth=1)
        else:
            if(use_old_latches_restoration_script):
                restore_multiclock_info_script = find_vtr_file("restore_multiclock_latch_information.pl")
            else:
                restore_multiclock_info_script = find_vtr_file("restore_multiclock_latch.pl")

            cmd = [restore_multiclock_info_script, pre_abc_blif.name, post_abc_raw_blif.name ,post_abc_blif.name]
            command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename="restore_latch" + str(i) + ".out", indent_depth=1)
        if(flow_type != 2):
            break
    
    cmd = [blackbox_latches_script, "--input", post_abc_blif.name,"--output",output_netlist.name,"--vanilla"]
    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename="restore_latch" + str(i) + ".out", indent_depth=1)
    
def run_ace(circuit_file, output_netlist, output_activity_file, command_runner, temp_dir=".", log_filename="ace.out", ace_exec=None):

    if ace_exec is None:
        ace_exec = find_vtr_file('ace', is_executable=True)

    cmd = [ace_exec,
           "-b", circuit_file.name,
           "-n", output_netlist.name,
           "-o", output_activity_file.name]

    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1)

def run_vpr_relax_W(architecture, circuit_name, circuit, command_runner=CommandRunner(), temp_dir=".", 
                    relax_W_factor=1.3, vpr_exec=None, verbosity=1, logfile_base="vpr",
                    vpr_args=None, output_netlist=None):
    """
    Runs VPR twice:
      1st: To find the minimum channel width
      2nd: At relaxed channel width (e.g. for critical path delay)

    Arguments
    ---------
        architecture: Architecture file
        circuit: Input circuit netlist
        command_runner: CommandRunner object
        temp_dir: Directory to run in

        relax_W_factor: Factor by which to relax minimum channel width for critical path delay routing
        verbosity: How much progress output to produce
        logfile_base: Base name for log files (e.g. "vpr" produces vpr.min_W.out, vpr.relaxed_W.out)
        vpr_args: Extra arguments for VPR
        vpr_exec: Path to the VPR executable
        output_netlist: Output implementation netlist to generate
    """
    if vpr_args is None:
        vpr_args = OrderedDict()

    mkdir_p(temp_dir)

    vpr_min_W_log = '.'.join([logfile_base, "min_W", "out"])
    vpr_relaxed_W_log = '.'.join([logfile_base, "relaxed_W", "out"])

    print_verbose(1, verbosity, "Running VPR (determining minimum channel width)" )

    run_vpr(architecture, circuit_name, circuit, command_runner, temp_dir, log_filename=vpr_min_W_log, vpr_exec=vpr_exec, vpr_args=vpr_args)

    if ('pack' in vpr_args or 'place' in vpr_args) and 'route' not in vpr_args:
        #Don't look for min W if routing was not run
        return


    min_W = determine_min_W(str(Path(temp_dir)  / vpr_min_W_log))

    relaxed_W = relax_W(min_W, relax_W_factor)

    print_verbose(1, verbosity, "Running VPR (at {fac}x relaxed minimum channel width)".format(fac=relax_W_factor))

    vpr_args['route'] = True #Re-route only
    vpr_args['route_chan_width'] = relaxed_W #At a fixed channel width

    #VPR does not support performing routing when fixed pins 
    # are specified, and placement is not run; so remove the option
    if 'fix_pins' in vpr_args:
        del vpr_args['fix_pins']

    run_vpr(architecture, circuit_name, circuit, command_runner, temp_dir, log_filename=vpr_relaxed_W_log, vpr_exec=vpr_exec, vpr_args=vpr_args)
    

def run_vpr(architecture, circuit_name, circuit, command_runner, temp_dir, output_netlist=None, log_filename="vpr.out", vpr_exec=None, vpr_args=None):
    """
    Runs VPR with the specified configuration
    """
    if vpr_args is None:
        vpr_args = OrderedDict()

    mkdir_p(temp_dir)

    if vpr_exec == None:
        vpr_exec = find_vtr_file('vpr', is_executable=True)

    cmd = [vpr_exec, architecture.name, circuit_name.name, "--circuit_file", circuit.name]

    #Enable netlist generation
    #if output_netlist:
        #vpr_args['gen_postsynthesis_netlist'] = output_netlist

    #Translate arbitrary keyword arguments into options for VPR
    for arg, value in vpr_args.iteritems():
        if value == True:
            cmd += ["--" + arg]
        elif value == False:
            pass
        else:
            cmd += ["--" + arg, str(value)]

    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1)

def run_abc_lec(reference_netlist, implementation_netlist, command_runner, temp_dir=".", log_filename="abc.lec.out", abc_exec=None):
    """
    Run Logical Equivalence Checking (LEC) between two netlists using ABC
    """
    mkdir_p(temp_dir)

    if abc_exec == None:
        abc_exec = find_vtr_file('abc', is_executable=True)

        abc_script = ['cec {ref} {imp}'.format(ref=reference_netlist, imp=implementation_netlist),
                      'sec {ref} {imp}'.format(ref=reference_netlist, imp=implementation_netlist),
                      ]
        abc_script = "; ".join(abc_script)

    cmd = [abc_exec, '-c', abc_script]

    output, returncode = command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1)

    #Check if ABC's LEC engine passed
    lec_passed = check_abc_lec_status(output)
    
    if lec_passed is None:
        raise InspectError("Could not determine Logical Equivalence Check status between {input} <-> {output}".format(input=reference_netlist, output=implementation_netlist), filename=log_filename)
    elif lec_passed is False:
        raise InspectError("Logical Equivalence Check failed between {input} <-> {output}".format(input=reference_netlist, output=implementation_netlist), filename=log_filename)
    
    assert lec_passed

def check_abc_lec_status(output):
    equivalent = None
    for line in output:
        if line.startswith("Networks are NOT EQUIVALENT"):
            equivalent = False
        elif line.startswith("Networks are equivalent"):
            equivalent = True

    #Returns None if could not determine LEC status
    return equivalent

def should_run_stage(stage, flow_start_stage, flow_end_stage):
    """
    Returns True if stage falls between flow_start_stage and flow_end_stage
    """
    if flow_start_stage <= stage <= flow_end_stage:
        return True
    return False

