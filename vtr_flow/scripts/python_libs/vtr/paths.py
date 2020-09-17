""" Paths for different VTR flow executables and resources """

import pathlib

# Path to the root repository directory
root_path = pathlib.Path(__file__).absolute().parent.parent.parent.parent.parent

# VTR Paths
vtr_flow_path = root_path / "vtr_flow"

# ODIN paths
odin_path = root_path / "ODIN_II"
odin_exe_path = odin_path / "odin_II"
odin_cfg_path = vtr_flow_path / "misc" / "basic_odin_config_split.xml"
odin_verify_path = odin_path / "verify_odin.sh"
odin_output_on_error_path = odin_path / "regression_test" / ".library" / "output_on_error.conf"

# ABC paths
abc_path = root_path / "abc"
abc_exe_path = abc_path / "abc"
abc_rc_path = abc_path / "abc.rc"

# ACE paths
ace_path = root_path / "ace2"
ace_exe_path = ace_path / "ace"
ace_extract_clk_from_blif_script_path = ace_path / "scripts" / "extract_clk_from_blif.py"

# VPR paths
vpr_path = root_path / "vpr"
vpr_exe_path = vpr_path / "vpr"

# Flow scripts
scripts_path = vtr_flow_path / "scripts"
run_vtr_flow_path = scripts_path / "run_vtr_flow.py"
flow_template_path = scripts_path / "flow_script_template.txt"

# Task files
tasks_path = vtr_flow_path / "tasks"
regression_tests_path = tasks_path / "regression_tests"

# Parsing files
parse_path = vtr_flow_path / "parse"
vtr_benchmarks_parse_path = parse_path / "parse_config" / "common" / "vtr_benchmarks.txt"
pass_requirements_path = parse_path / "pass_requirements"

# Other scripts
blackbox_latches_script_path = scripts_path / "blackbox_latches.pl"
restore_multiclock_latch_old_script_path = scripts_path / "restore_multiclock_latch_information.pl"
restore_multiclock_latch_script_path = scripts_path / "restore_multiclock_latch.pl"
valgrind_supp = vpr_path / "valgrind.supp"
lsan_supp = vpr_path / "lsan.supp"
