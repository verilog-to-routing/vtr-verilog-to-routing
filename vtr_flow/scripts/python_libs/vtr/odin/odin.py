import shutil
from pathlib import Path
from vtr import  mkdir_p, find_vtr_file, file_replace, determine_memory_addr_width, verify_file

def run(architecture_file, circuit_file, 
             output_netlist, 
             command_runner, 
             temp_dir=".", 
             log_filename="odin.out", 
             odin_exec=None, 
             odin_config=None, 
             min_hard_mult_size=3, 
             min_hard_adder_size=1):
    mkdir_p(temp_dir)

    verify_file(architecture_file, "Architecture")
    verify_file(circuit_file, "Circuit")
    verify_file(output_netlist, "Output netlist", False)

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