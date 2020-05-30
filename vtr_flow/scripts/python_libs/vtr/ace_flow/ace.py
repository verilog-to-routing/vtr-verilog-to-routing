from vtr import find_vtr_file
from pathlib import Path
def run(circuit_file, old_netlist, output_netlist, output_activity_file, command_runner, temp_dir=".", log_filename="ace.out", ace_exec=None, ace_seed = 1):
    if(not isinstance(circuit_file,Path)):
        circuit_file = Path(circuit_file)
        
    if(not isinstance(old_netlist,Path)):
        old_netlist = Path(old_netlist)

    if(not isinstance(output_netlist,Path)):
        output_netlist = Path(output_netlist)

    if(not isinstance(output_activity_file,Path)):
        output_activity_file = Path(output_activity_file)

    ace_clk_file = Path(temp_dir) / "ace_clk.txt"
    ace_raw = Path(temp_dir) / (circuit_file.with_suffix('').stem + ".raw.ace.blif")
    if ace_exec is None:
        ace_exec = find_vtr_file('ace')

    ace_extraction_file = Path(find_vtr_file("extract_clk_from_blif.py"))

    cmd =[str(ace_extraction_file), ace_clk_file.name, circuit_file.name]
    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename="ace_clk_extraction.out", indent_depth=1)
    ace_clk = ""
    with ace_clk_file.open('r') as f:
         ace_clk = f.readline().strip('\n')
    cmd = [ace_exec,
           "-b", circuit_file.name,
           "-c", ace_clk,
           "-n", ace_raw.name,
           "-o", output_activity_file.name,
           "-s", str(ace_seed)]

    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1)

    clock_script = find_vtr_file("restore_multiclock_latch.pl")
    
    cmd = [ clock_script, old_netlist.name, ace_raw.name, output_netlist.name]
    command_runner.run_system_command(cmd, temp_dir=temp_dir, log_filename="ace_clk_restore.out", indent_depth=1)

