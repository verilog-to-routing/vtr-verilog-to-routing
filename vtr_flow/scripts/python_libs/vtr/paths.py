""" Paths for different VTR flow executables and resources """

import pathlib

# Path to the root repository directory
root_path = pathlib.Path(__file__).absolute().parent.parent.parent.parent.parent

vtr_flow_path = root_path / "vtr_flow"
build_path = root_path / "build"

abc_exe_path = root_path / "abc" / "abc"
abc_rc_path = root_path / "abc" / "abc.rc"
