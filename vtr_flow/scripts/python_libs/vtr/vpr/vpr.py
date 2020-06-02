import shutil
from pathlib import Path
from vtr import  mkdir_p, find_vtr_file, CommandRunner, print_verbose, relax_W, determine_lut_size, determine_min_W, verify_file

def run_relax_W(architecture, circuit_name, circuit, command_runner=CommandRunner(), temp_dir=".", 
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

    verify_file(architecture, "Architecture")
    verify_file(circuit_name, "Circuit")
    verify_file(circuit, "Circuit")

    vpr_min_W_log = '.'.join([logfile_base, "min_W", "out"])
    vpr_relaxed_W_log = '.'.join([logfile_base, "relaxed_W", "out"])

    run(architecture, circuit_name, circuit, command_runner, temp_dir, log_filename=vpr_min_W_log, vpr_exec=vpr_exec, vpr_args=vpr_args)

    if ('pack' in vpr_args or 'place' in vpr_args) and 'route' not in vpr_args:
        #Don't look for min W if routing was not run
        return


    min_W = determine_min_W(str(Path(temp_dir)  / vpr_min_W_log))

    relaxed_W = relax_W(min_W, relax_W_factor)

    vpr_args['route'] = True #Re-route only
    vpr_args['route_chan_width'] = relaxed_W #At a fixed channel width

    #VPR does not support performing routing when fixed pins 
    # are specified, and placement is not run; so remove the option
    if 'fix_pins' in vpr_args:
        del vpr_args['fix_pins']

    run(architecture, circuit_name, circuit, command_runner, temp_dir, log_filename=vpr_relaxed_W_log, vpr_exec=vpr_exec, vpr_args=vpr_args)
    

def run(architecture, circuit_name, circuit, command_runner, temp_dir, output_netlist=None, log_filename="vpr.out", vpr_exec=None, vpr_args=None):
    """
    Runs VPR with the specified configuration
    """
    
    if vpr_args is None:
        vpr_args = OrderedDict()

    mkdir_p(temp_dir)

    if vpr_exec == None:
        vpr_exec = find_vtr_file('vpr', is_executable=True)

    verify_file(architecture, "Architecture")
    verify_file(circuit_name, "Circuit")
    verify_file(circuit, "Circuit")

    cmd = [vpr_exec, architecture.name, circuit_name.stem, "--circuit_file", circuit.name]

    #Enable netlist generation
    #if output_netlist:
        #vpr_args['gen_postsynthesis_netlist'] = output_netlist

    #Translate arbitrary keyword arguments into options for VPR
    for arg, value in vpr_args.items():
        if value == True:
            cmd += ["--" + arg]
        elif value == False:
            pass
        else:
            cmd += ["--" + arg, str(value)]

    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1)