""" Paths for different VTR flow executables and resources """

import pathlib

# Path to the root repository directory
root_path = pathlib.Path(__file__).absolute().parent.parent.parent.parent.parent

vtr_flow_path = root_path / "vtr_flow"

scripts_path = root_path / "scripts"
build_path = root_path / "build"

# ABC exe paths
abc_exe_path = root_path / "abc" / "abc"
abc_rc_path = root_path / "abc" / "abc.rc"

# ACE exe paths
ace_exe_path = root_path / "ace2" / "ace"
ace_extract_clk_from_blif_script_path = root_path / "ace2" / "scripts" / "extract_clk_from_blif.py"

# Other scripts
blackbox_latches_script_path = scripts_path / "blackbox_latches.pl"
restore_multiclock_latch_old_script_path = scripts_path / "restore_multiclock_latch_information.pl"
restore_multiclock_latch_script_path = scripts_path / "restore_multiclock_latch.pl"

