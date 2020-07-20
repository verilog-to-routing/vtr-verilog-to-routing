import shutil
from pathlib import Path
from vtr import  mkdir_p, find_vtr_file, CommandRunner, print_verbose, relax_W, determine_lut_size, determine_min_W, verify_file
from vtr.error import InspectError

def run_relax_W(architecture, circuit, circuit_name=None, command_runner=CommandRunner(), temp_dir=".", 
                    relax_W_factor=1.3, vpr_exec=None, logfile_base="vpr",
                    vpr_args=None, output_netlist=None):
    """
    Runs VPR twice:

      1st: To find the minimum channel width

      2nd: At relaxed channel width (e.g. for critical path delay)
    
    .. note :: Usage: vtr.vpr.run_relax_W(<architecture_file>,<circuit_name>,<circuit_file>,[OPTIONS])

    Arguments
    =========
        architecture: 
            Architecture file
        
        circuit: 
            Input circuit netlist

    Other Parameters
    ----------------
        circuit_name: 
            Name of the circuit file

        command_runner: 
            CommandRunner object
        
        temp_dir: 
            Directory to run in
        
        relax_W_factor: 
            Factor by which to relax minimum channel width for critical path delay routing
        
        vpr_exec: 
            Path to the VPR executable
        
        logfile_base: 
            Base name for log files (e.g. "vpr" produces vpr.min_W.out, vpr.relaxed_W.out)
        
        vpr_args: 
            Extra arguments for VPR
        
        output_netlist: 
            Output implementation netlist to generate
    """
    if vpr_args is None:
        vpr_args = OrderedDict()

    mkdir_p(temp_dir)

    #Verify that files are Paths or convert them to Paths and check that they exist
    architecture = verify_file(architecture, "Architecture")
    circuit = verify_file(circuit, "Circuit")

    vpr_min_W_log = '.'.join([logfile_base, "out"])
    vpr_relaxed_W_log = '.'.join([logfile_base, "crit_path", "out"])
    max_router_iterations = None

    if "max_router_iterations" in vpr_args:
        max_router_iterations = vpr_args["max_router_iterations"]
        del vpr_args["max_router_iterations"]

    if "write_rr_graph" in vpr_args:
        del vpr_args["write_rr_graph"]

    if "analysis" in vpr_args:
        del vpr_args["analysis"]

    if "route" in vpr_args:
        del vpr_args["route"]

    if vpr_exec == None:
        vpr_exec = find_vtr_file('vpr', is_executable=True)

    run(architecture, circuit, circuit_name, command_runner, temp_dir, log_filename=vpr_min_W_log, vpr_exec=vpr_exec, vpr_args=vpr_args)

    if ('pack' in vpr_args or 'place' in vpr_args) and 'route' not in vpr_args:
        #Don't look for min W if routing was not run
        return
    if max_router_iterations:
        vpr_args["max_router_iterations"]=max_router_iterations
    min_W = determine_min_W(str(Path(temp_dir)  / vpr_min_W_log))

    relaxed_W = relax_W(min_W, relax_W_factor)

    vpr_args['route'] = True #Re-route only
    vpr_args['route_chan_width'] = relaxed_W #At a fixed channel width

    #VPR does not support performing routing when fixed pins 
    # are specified, and placement is not run; so remove the option

    run(architecture, circuit, circuit_name, command_runner, temp_dir, log_filename=vpr_relaxed_W_log, vpr_exec=vpr_exec, vpr_args=vpr_args, check_for_second_run=False)
    

def run(architecture, circuit, circuit_name=None, command_runner=CommandRunner(), temp_dir=".", output_netlist=None,
 log_filename="vpr.out", vpr_exec=None, vpr_args=None,check_for_second_run=True,rr_graph_ext=".xml",):
    """
    Runs VPR with the specified configuration

    .. note :: Usage: vtr.vpr.run(<architecture>,<circuit_name>,<circuit>,[OPTIONS])

    Arguments
    =========
        architecture: 
            Architecture file
        
        circuit:
            Input circuit file
    
    Other Parameters
    ----------------
        circuit_name: 
            Name of the circuit file

        command_runner: 
            CommandRunner object
        
        temp_dir:
            Directory to run in

        output_netlist: 
            Output implementation netlist to generate
        
        log_filename : 
            File to log result to
        
        vpr_exec: 
            Path to the VPR executable
        
        vpr_args: 
            Extra arguments for VPR
        
        check_for_second_run: 
            enables checking for arguments in vpr_args that require a second run of VPR ie analysis

    """
    
    if vpr_args is None:
        vpr_args = OrderedDict()

    mkdir_p(temp_dir)

    if vpr_exec == None:
        vpr_exec = find_vtr_file('vpr', is_executable=True)

    #Verify that files are Paths or convert them to Paths and check that they exist
    architecture = verify_file(architecture, "Architecture")
    circuit = verify_file(circuit, "Circuit")
    cmd = []
    if circuit_name:
        cmd = [vpr_exec, architecture.name, circuit_name, "--circuit_file", circuit.name]
    else:
        cmd =  [vpr_exec, architecture.name, circuit.name]
    #Enable netlist generation
    #if output_netlist:
        #vpr_args['gen_postsynthesis_netlist'] = output_netlist

    #Translate arbitrary keyword arguments into options for VPR
    do_second_run = False
    second_run_args = vpr_args

    if check_for_second_run and  "write_rr_graph" in vpr_args:
        do_second_run = True

    if check_for_second_run and "analysis" in vpr_args:
        do_second_run = True
        del vpr_args["analysis"]

    if check_for_second_run and "route" in vpr_args:
        do_second_run = True
        del vpr_args["route"]

    for arg, value in vpr_args.items():
        if value == True:
            cmd += ["--" + arg]
        elif value == False:
            pass
        else:
            if isinstance(value,list):
                cmd += ["--" + arg]
                for i in range(len(value)):
                    cmd += [str(value[i])]
            else:
                cmd += ["--" + arg, str(value)]

    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1)

    if(do_second_run):
        rr_graph_out_file = ""
        if "write_rr_graph" in second_run_args:
            rr_graph_out_file = second_run_args["write_rr_graph"]
            rr_graph_ext = Path(rr_graph_out_file).suffix

        rr_graph_out_file2 = "rr_graph2" + rr_graph_ext
        if "write_rr_graph" in second_run_args:
            second_run_args["read_rr_graph"] = rr_graph_out_file
            second_run_args["write_rr_graph"] = rr_graph_out_file2

        second_run_log_file = "vpr_second_run.out"
        cmd = []
        if circuit_name:
            cmd = [vpr_exec, architecture.name, circuit_name, "--circuit_file", circuit.name]
        else:
            cmd =  [vpr_exec, architecture.name, circuit.name]
        for arg, value in vpr_args.items():
            if value == True:
                cmd += ["--" + arg]
            elif value == False:
                pass
            else:
                cmd += ["--" + arg, str(value)]

        command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=second_run_log_file, indent_depth=1)

        if "write_rr_graph" in second_run_args:
            cmd = ["diff", rr_graph_out_file, rr_graph_out_file2]
            output, diff_result = command_runner.run_system_command(cmd,temp_dir,log_filename="diff.rr_graph.out", indent_depth=1)
            if diff_result:
                raise InspectError("failed: vpr (RR Graph XML output not consistent when reloaded)")

def cmp_full_vs_incr_STA(architecture,circuit,circuit_name=None,command_runner=CommandRunner(),vpr_args=None,rr_graph_ext=".xml",temp_dir=".",vpr_exec=None):
    """
    Sanity check that full STA and the incremental STA produce the same *.net, *.place, *.route files as well as identical timing report files

    .. note :: Usage: vtr.vpr.cmp_full_vs_incr_STA(<architecture>,<circuit_name>,<circuit>,[OPTIONS])

    Arguments
    =========
        architecture: 
            Architecture file
        
        circuit: 
            Input circuit file
    
    Other Parameters
    ----------------
        circuit_name: 
            Name of the circuit file

        command_runner: 
            CommandRunner object
        
        temp_dir: 
            Directory to run in
        
        vpr_exec: 
            Path to the VPR executable
        
        vpr_args: 
            Extra arguments for VPR
        
    """

    #Verify that files are Paths or convert them to Paths and check that they exist
    architecture = verify_file(architecture, "Architecture")
    circuit = verify_file(circuit, "Circuit")
    if not circuit_name:
        circuit_name = circuit.stem
    default_output_filenames = [
            "{}.net".format(circuit_name),
            "{}.place".format(circuit_name),
            "{}.route".format(circuit_name),
            "report_timing.setup.rpt",
			"report_timing.hold.rpt",
			"report_unconstrained_timing.setup.rpt",
			"report_unconstrained_timing.hold.rpt"]

    dif_exec = "diff"
    move_exec = "mv"
    # The full STA flow should have already been run
    # directly rename the output files
    for filename in default_output_filenames:
        cmd = [move_exec,filename,"full_sta_{}".format(filename)]
        command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename="move.out", indent_depth=1)
    
    # run incremental STA flow
    incremental_vpr_args = vpr_args
    incremental_vpr_args["timing_update_type"]="incremental"
    fixed_W_log_file = "vpr.incr_sta.out"

    run(architecture, circuit, circuit_name, command_runner, temp_dir, log_filename=fixed_W_log_file, vpr_exec=vpr_exec, vpr_args=incremental_vpr_args, check_for_second_run=False)

    # Rename the incremental STA output files
    for filename in default_output_filenames:
        cmd = [move_exec,filename,"incremental_sta_{}".format(filename)]
        command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename="move.out", indent_depth=1)
    
    failed_msg = "Failed with these files (not identical):"
    identical = True

    for filename in default_output_filenames:
        cmd=[dif_exec,"full_sta_{}".format(filename),"incremental_sta_{}".format(filename)]
        cmd_output,cmd_return_code = command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename="diff.out", indent_depth=1)
        if(cmd_return_code):
            identical = False
            failed_msg += " {}".format(filename)
    
    if not identical:
        raise InspectError(failed_msg)
