from vtr import find_vtr_file
def run(circuit_file, output_netlist, output_activity_file, command_runner, temp_dir=".", log_filename="ace.out", ace_exec=None):

    if ace_exec is None:
        ace_exec = find_vtr_file('ace', is_executable=True)

    cmd = [ace_exec,
           "-b", circuit_file.name,
           "-n", output_netlist.name,
           "-o", output_activity_file.name]

    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1)