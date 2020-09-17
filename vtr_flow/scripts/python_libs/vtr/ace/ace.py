"""
    Module to run ACE with its various options
"""
from pathlib import Path
from vtr import verify_file, CommandRunner, paths

# pylint: disable=too-many-arguments
def run(
    circuit_file,
    old_netlist,
    output_netlist,
    output_activity_file,
    command_runner=CommandRunner(),
    temp_dir=Path("."),
    log_filename="ace.out",
    ace_exec=None,
    ace_seed=1,
):
    """
    Runs ACE for activity estimation

    .. note :: Usage: vtr.ace.run(<circuit_file>,<output_netlist>,<output_activity_file>,[OPTIONS])

    Arguments
    =========
        circuit_file :
            Circuit file to optimize

        old_netlist :
            netlist to be anylized

        output_netlist :
            File name to output the resulting circuit to

        output_activity_file :
            The output activity file

    Other Parameters
    ----------------
        command_runner :
            A CommandRunner object used to run system commands

        temp_dir :
            Directory to run in (created if non-existent)

        log_filename :
            File to log result to

        ace_exec :
            ACE executable to be run

        ace_seed :
            The ACE seed
    """
    temp_dir = Path(temp_dir) if not isinstance(temp_dir, Path) else temp_dir
    # Verify that files are Paths or convert them to Paths and check that they exist
    circuit_file = verify_file(circuit_file, "Circuit")
    old_netlist = verify_file(old_netlist, "Previous netlist")
    output_netlist = verify_file(output_netlist, "Output netlist", should_exist=False)
    output_activity_file = verify_file(output_activity_file, "Output activity", should_exist=False)

    ace_clk_file = temp_dir / "ace_clk.txt"
    ace_raw = temp_dir / (circuit_file.with_suffix("").stem + ".raw.ace.blif")
    if ace_exec is None:
        ace_exec = str(paths.ace_exe_path)

    cmd = [str(paths.ace_extract_clk_from_blif_script_path), ace_clk_file.name, circuit_file.name]
    command_runner.run_system_command(
        cmd, temp_dir=temp_dir, log_filename="ace_clk_extraction.out", indent_depth=1
    )
    ace_clk = ""
    with ace_clk_file.open("r") as file:
        ace_clk = file.readline().strip("\n")
    cmd = [
        ace_exec,
        "-b",
        circuit_file.name,
        "-c",
        ace_clk,
        "-n",
        ace_raw.name,
        "-o",
        output_activity_file.name,
        "-s",
        str(ace_seed),
    ]

    command_runner.run_system_command(
        cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1
    )

    clock_script = str(paths.restore_multiclock_latch_script_path)

    cmd = [clock_script, old_netlist.name, ace_raw.name, output_netlist.name]
    command_runner.run_system_command(
        cmd, temp_dir=temp_dir, log_filename="ace_clk_restore.out", indent_depth=1
    )
    # pylint: enable=too-many-arguments
