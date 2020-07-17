import shutil
from pathlib import Path
from vtr import  mkdir_p, find_vtr_file, file_replace, determine_memory_addr_width, verify_file, CommandRunner

def run(architecture_file, circuit_file, 
             output_netlist, 
             command_runner=CommandRunner(),
             temp_dir=".", 
             odin_args="--adder_type default",
             log_filename="odin.out", 
             odin_exec=None, 
             odin_config=None, 
             min_hard_mult_size=3, 
             min_hard_adder_size=1):
    """
    Runs ODIN II on the specified architecture file and circuit file

    .. note :: Usage: vtr.odin.run(<architecture_file>,<circuit_file>,<output_netlist>,[OPTIONS])

    Arguments
    =========
        architecture_file : 
            Architecture file to target
        
        circuit_file : 
            Circuit file to optimize
        
        output_netlist : 
            File name to output the resulting circuit to

    Other Parameters
    ----------------
        command_runner : 
            A CommandRunner object used to run system commands
        
        temp_dir : 
            Directory to run in (created if non-existent)
        
        odin_args: 
            A dictionary of keyword arguments to pass on to ODIN II 
        
        log_filename : 
            File to log result to
        
        odin_exec: 
            ODIN II executable to be run
        
        odin_config: 
            The ODIN II configuration file
        
        min_hard_mult_size : 
            Tells ODIN II the minimum multiplier size that should be implemented using hard multiplier (if available)
        
        min_hard_adder_size : 
            Tells ODIN II the minimum adder size that should be implemented using hard adder (if available).
        
    """
    mkdir_p(temp_dir)

    if odin_args == None:
        odin_args = OrderedDict()

    #Verify that files are Paths or convert them to Paths and check that they exist
    architecture_file = verify_file(architecture_file, "Architecture")
    circuit_file = verify_file(circuit_file, "Circuit")
    output_netlist = verify_file(output_netlist, "Output netlist", False)

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
    disable_odin_xml = False
    if("disable_odin_xml" in odin_args):
        disable_odin_xml=True
        del odin_args["disable_odin_xml"]
    use_odin_simulation = False
    if("use_odin_simulation" in odin_args):
        use_odin_simulation=True
        del odin_args["use_odin_simulation"]
        
    cmd = [odin_exec, "-c", odin_config]
    for arg, value in odin_args.items():
        if value == True:
            cmd += ["--" + arg]
        elif value == False:
            pass
        else:
            cmd += ["--" + arg, str(value)]
    cmd += ["-U0"]
    if(disable_odin_xml):
        cmd += ["-a",architecture_file.name, "-V", circuit_file.name, "-o",output_netlist.name]

    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1)

    if(use_odin_simulation):
        sim_dir = Path(temp_dir) / "simulation_init"
        sim_dir.mkdir()
        cmd = [odin_exec, "-b", output_netlist.name, -a, architecture_file.name, "-sim_dir", str(sim_dir),"-g", "100", "--best_coverage", "-U0"]
        command_runner.run_system_command(cmd,temp_dir=temp_dir, log_filename="sim_produce_vector.out",indent_depth=1)