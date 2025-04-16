"""
Module that contains the task functions
"""

import itertools

from pathlib import Path
from pathlib import PurePath
from shlex import split

from typing import List, Tuple

from vtr import (
    VtrError,
    InspectError,
    load_list_file,
    load_parse_results,
    get_existing_run_dir,
    get_next_run_dir,
    get_active_run_dir,
    find_task_dir,
    load_script_param,
    paths,
)


# pylint: disable=too-many-instance-attributes, too-many-arguments, too-many-locals, too-few-public-methods
class TaskConfig:
    """
    An object representing a task config file
    """

    def __init__(
        self,
        task_name,
        config_dir,
        circuits_dir,
        archs_dir,
        circuit_list_add,
        arch_list_add,
        parse_file,
        includes_dir=None,
        include_list_add=None,
        second_parse_file=None,
        script_path=None,
        script_params=None,
        script_params_common=None,
        script_params_list_add=None,
        pass_requirements_file=None,
        sdc_dir=None,
        noc_traffic_list_type="outer_product",
        noc_traffic_list_add=None,
        noc_traffics_dir=None,
        place_constr_dir=None,
        qor_parse_file=None,
        cmos_tech_behavior=None,
        pad_file=None,
        additional_files=None,
        additional_files_list_add=None,
        circuit_constraint_list_add=None,
    ):
        self.task_name = task_name
        self.config_dir = config_dir
        self.circuit_dir = circuits_dir
        self.arch_dir = archs_dir
        self.circuits = circuit_list_add
        self.archs = arch_list_add
        self.include_dir = includes_dir
        self.includes = include_list_add
        self.parse_file = parse_file
        self.second_parse_file = second_parse_file
        self.script_path = script_path
        self.script_params = script_params
        self.script_params_common = script_params_common
        self.script_params_list_add = script_params_list_add
        self.pass_requirements_file = pass_requirements_file
        self.sdc_dir = sdc_dir
        self.noc_traffic_list_type = noc_traffic_list_type
        self.noc_traffics = [None] if noc_traffic_list_add is None else noc_traffic_list_add
        self.noc_traffic_dir = noc_traffics_dir
        self.place_constr_dir = place_constr_dir
        self.qor_parse_file = qor_parse_file
        self.cmos_tech_behavior = cmos_tech_behavior
        self.pad_file = pad_file
        self.additional_files = additional_files
        self.additional_files_list_add = additional_files_list_add
        self.circuit_constraints = parse_circuit_constraint_list(
            circuit_constraint_list_add, self.circuits, self.archs
        )


# pylint: enable=too-few-public-methods


class Job:
    """
    A class to store the necessary information for a job that needs to be run.
    """

    def __init__(
        self,
        task_name,
        arch,
        circuit,
        include,
        script_params,
        work_dir,
        run_command,
        parse_command,
        second_parse_command,
        qor_parse_command,
    ):
        self._task_name = task_name
        self._arch = arch
        self._circuit = circuit
        self._include = include
        self._script_params = script_params
        self._run_command = run_command
        self._parse_command = parse_command
        self._second_parse_command = second_parse_command
        self._qor_parse_command = qor_parse_command
        self._work_dir = work_dir

    def task_name(self):
        """
        return the task name of the job
        """
        return self._task_name

    def arch(self):
        """
        return the architecture file name of the job
        """
        return self._arch

    def circuit(self):
        """
        return the circuit file name of the job
        """
        return self._circuit

    def include(self):
        """
        return the list of include files (.v/.vh) of the job.
        """
        return self._include

    def script_params(self):
        """
        return the script parameter of the job
        """
        return self._script_params

    def job_name(self):
        """
        return the name of the job
        """
        return str(PurePath(self.arch()) / self.circuit() / self.script_params())

    def run_command(self):
        """
        return the run command of the job
        """
        return self._run_command

    def parse_command(self):
        """
        return the parse command of the job
        """
        return self._parse_command

    def second_parse_command(self):
        """
        return the second parse command of the job
        """
        return self._second_parse_command

    def qor_parse_command(self):
        """
        return the qor parse command of the job
        """
        return self._qor_parse_command

    def work_dir(self, run_dir: str) -> str:
        """
        return the work directory of the job
        """
        return str(PurePath(run_dir).joinpath(self._work_dir))


# pylint: enable=too-many-instance-attributes


def load_task_config(config_file) -> TaskConfig:
    """
    Load task config information
    """
    # Load the file stripping comments
    values = load_list_file(config_file)

    # Keys that can appear only once
    unique_keys = set(
        [
            "circuits_dir",
            "includes_dir",
            "archs_dir",
            "additional_files",
            "parse_file",
            "script_path",
            "script_params",
            "script_params_common",
            "pass_requirements_file",
            "sdc_dir",
            "noc_traffic_list_type",
            "noc_traffics_dir",
            "place_constr_dir",
            "qor_parse_file",
            "cmos_tech_behavior",
            "pad_file",
        ]
    )

    # Keys that are repeated to create lists
    repeated_key_pattern = "_list_add"

    # Keys that are required
    required_keys = set(
        ["circuits_dir", "archs_dir", "circuit_list_add", "arch_list_add", "parse_file"]
    )

    # Interpret the file
    key_values = {}
    for line in values:
        # Split the key and value using only the first equal sign. This allows
        # the value to have an equal sign.
        key, value = line.split("=", 1)

        # Trim whitespace
        key = key.strip()
        value = value.strip()

        if key in unique_keys:
            if key not in key_values:
                key_values[key] = value
            elif key == "parse_file":
                key_values["second_parse_file"] = value
            else:
                raise VtrError(
                    "Duplicate {key} in config file {file}".format(key=key, file=config_file)
                )

        elif repeated_key_pattern in key:
            if key not in key_values:
                key_values[key] = []
            if key == "script_params_list_add":
                key_values[key] += [value]
            else:
                key_values[key].append(value)

        else:
            # All valid keys should have been collected by now
            raise VtrError(
                "Unrecognized key '{key}' in config file {file}".format(key=key, file=config_file)
            )

    # We split the script params into a list
    if "script_params" in key_values:
        key_values["script_params"] = split(key_values["script_params"])

    if "script_params_common" in key_values:
        key_values["script_params_common"] = split(key_values["script_params_common"])

    check_required_fields(config_file, required_keys, key_values)
    check_include_fields(config_file, key_values)

    # Useful meta-data about the config
    config_dir = str(Path(config_file).parent)
    key_values["config_dir"] = config_dir
    key_values["task_name"] = Path(config_dir).parent.name

    # Create the task config object
    return TaskConfig(**key_values)


def check_required_fields(config_file, required_keys, key_values):
    """
    Check that all required fields were specified
    """
    for required_key in required_keys:
        if required_key not in key_values:
            raise VtrError(
                "Missing required key '{key}' in config file {file}".format(
                    key=required_key, file=config_file
                )
            )


def check_include_fields(config_file, key_values):
    """
    Check that includes_dir was specified if some files to include
    in the designs (include_list_add) was specified.
    """
    if "include_list_add" in key_values:
        if "includes_dir" not in key_values:
            raise VtrError(
                "Missing required key '{key}' in config file {file}".format(
                    key="includes_dir", file=config_file
                )
            )


def parse_circuit_constraint_list(circuit_constraint_list, circuits_list, arch_list) -> dict:
    """
    Parse the circuit constraints passed in via the config file.
    Circuit constraints are expected to have the following syntax:
        (<circuit>, <constr_key>=<constr_val>)
    This function generates a dictionary which can be accessed:
        circuit_constraints[circuit][constr_key]
    If this dictionary returns "None", then the circuit is unconstrained for
    that key.
    """

    # Constraint keys that can be specified.
    circuit_constraint_keys = set(
        [
            "arch",
            "device",
            "constraints",
            "route_chan_width",
        ]
    )

    # Initialize the dictionary to be unconstrained for all circuits and keys.
    res_circuit_constraints = {
        circuit: {constraint_key: None for constraint_key in circuit_constraint_keys}
        for circuit in circuits_list
    }

    # If there were no circuit constraints passed by the user, return dictionary
    # of Nones.
    if circuit_constraint_list is None:
        return res_circuit_constraints

    # Parse the circuit constraint list
    for circuit_constraint in circuit_constraint_list:
        # Remove the round brackets.
        if circuit_constraint[0] != "(" or circuit_constraint[-1] != ")":
            raise VtrError(f'Circuit constraint syntax error: "{circuit_constraint}"')
        circuit_constraint = circuit_constraint[1:-1]
        # Split the circuit and the constraint
        split_constraint_line = circuit_constraint.split(",")
        if len(split_constraint_line) != 2:
            raise VtrError(f'Circuit constraint has too many arguments: "{circuit_constraint}"')
        circuit = split_constraint_line[0].strip()
        constraint = split_constraint_line[1].strip()
        # Check that the circuit actually exists.
        if circuit not in circuits_list:
            raise VtrError(f'Cannot constrain circuit "{circuit}", circuit has not been added')
        # Parse the constraint
        split_constraint = constraint.split("=")
        if len(split_constraint) != 2:
            raise VtrError(f'Circuit constraint syntax error: "{circuit_constraint}"')
        constr_key = split_constraint[0].strip()
        constr_val = split_constraint[1].strip()
        # Check that the constr_key is valid.
        if constr_key not in circuit_constraint_keys:
            raise VtrError(f'Invalid constraint "{constr_key}" used on circuit "{circuit}"')
        # In the case of arch constraints, make sure this arch exists.
        if constr_key == "arch" and constr_val not in arch_list:
            raise VtrError(f'Cannot constrain arch "{constr_key}", arch has not been added')
        # Make sure this circuit is not already constrained with this constr_arg
        if res_circuit_constraints[circuit][constr_key] is not None:
            raise VtrError(f'Circuit "{circuit}" cannot be constrained more than once')
        # Add the constraint for this circuit
        res_circuit_constraints[circuit][constr_key] = constr_val

    return res_circuit_constraints


def shorten_task_names(configs, common_task_prefix):
    """
    Shorten the task names of the configs by remove the common task prefix.
    """
    new_configs = []
    for config in configs:
        config.task_name = config.task_name.replace(common_task_prefix, "")
        new_configs += [config]
    return new_configs


def find_longest_task_description(configs):
    """
    Finds the longest task description in the list of configurations.
    This is used for output spacing.
    """
    longest = 0
    for config in configs:
        for arch, circuit in itertools.product(config.archs, config.circuits):
            if config.script_params_list_add:
                for param in config.script_params_list_add:
                    arch_circuit_len = len(str(PurePath(arch) / circuit / "common_" / param))
                    if arch_circuit_len > longest:
                        longest = arch_circuit_len
            else:
                arch_circuit_len = len(str(PurePath(arch) / circuit / "common"))
                if arch_circuit_len > longest:
                    longest = arch_circuit_len
    return longest


def get_work_dir_addr(arch, circuit, noc_traffic):
    """Get the work directory address under under run_dir"""
    work_dir = None
    if noc_traffic:
        work_dir = str(PurePath(arch).joinpath(circuit).joinpath(noc_traffic))
    else:
        work_dir = str(PurePath(arch).joinpath(circuit))

    return work_dir


def create_second_parse_cmd(config):
    """Create the parse command to run the second time"""
    second_parse_cmd = None
    if config.second_parse_file:
        second_parse_cmd = [
            resolve_vtr_source_file(
                config,
                config.second_parse_file,
                str(PurePath("parse").joinpath("parse_config")),
            )
        ]

    return second_parse_cmd


# pylint: disable=too-many-branches
def create_cmd(
    abs_circuit_filepath, abs_arch_filepath, config, args, circuit, noc_traffic
) -> Tuple:
    """Create the command to run the task"""
    # Collect any extra script params from the config file
    cmd = [abs_circuit_filepath, abs_arch_filepath]

    # Resolve and collect all include paths in the config file
    # as -include ["include1", "include2", ..]
    includes = []
    if config.includes:
        cmd += ["-include"]
        for include in config.includes:
            abs_include_filepath = resolve_vtr_source_file(config, include, config.include_dir)
            includes.append(abs_include_filepath)

        cmd += includes

    # Check if additional architectural data files are present
    if config.additional_files_list_add:
        for additional_file in config.additional_files_list_add:
            flag, file_name = additional_file.split(",")

            cmd += [flag]
            cmd += [resolve_vtr_source_file(config, file_name, config.arch_dir)]

    if hasattr(args, "show_failures") and args.show_failures:
        cmd += ["-show_failures"]
    cmd += config.script_params if config.script_params else []
    cmd += config.script_params_common if config.script_params_common else []
    cmd += (
        args.shared_script_params
        if hasattr(args, "shared_script_params") and args.shared_script_params
        else []
    )

    # Apply any special config based parameters
    if config.cmos_tech_behavior:
        cmd += [
            "-cmos_tech",
            resolve_vtr_source_file(config, config.cmos_tech_behavior, "tech"),
        ]

    cmd += (
        ["--fix_pins", resolve_vtr_source_file(config, config.pad_file)] if config.pad_file else []
    )

    if config.sdc_dir:
        sdc_name = "{}.sdc".format(Path(circuit).stem)
        sdc_file = resolve_vtr_source_file(config, sdc_name, config.sdc_dir)

        cmd += ["-sdc_file", "{}".format(sdc_file)]

    if config.place_constr_dir:
        place_constr_name = "{}.place".format(Path(circuit).stem)
        place_constr_file = resolve_vtr_source_file(
            config, place_constr_name, config.place_constr_dir
        )

        cmd += ["--fix_clusters", "{}".format(place_constr_file)]

    # parse_vtr_task doesn't have these in args, so use getattr here
    if getattr(args, "write_rr_graphs", None):
        cmd += [
            "--write_rr_graph",
            "{}.rr_graph.xml".format(Path(circuit).stem),
        ]  # Use XML format instead of capnp (see #2352)

    if getattr(args, "write_lookaheads", None):
        cmd += ["--write_router_lookahead", "{}.lookahead.bin".format(Path(circuit).stem)]

    if getattr(args, "write_rr_graphs", None) or getattr(args, "write_lookaheads", None):
        # Don't trigger a second run, we just want the files
        cmd += ["-no_second_run"]

    parse_cmd = None
    qor_parse_command = None
    if config.parse_file:
        parse_cmd = [
            resolve_vtr_source_file(
                config,
                config.parse_file,
                str(PurePath("parse").joinpath("parse_config")),
            )
        ]

    second_parse_cmd = create_second_parse_cmd(config)

    if config.qor_parse_file:
        qor_parse_command = [
            resolve_vtr_source_file(
                config,
                config.qor_parse_file,
                str(PurePath("parse").joinpath("qor_config")),
            )
        ]
    # We specify less verbosity to the sub-script
    # This keeps the amount of output reasonable
    if hasattr(args, "verbosity") and max(0, args.verbosity - 1):
        cmd += ["-verbose"]

    if noc_traffic:
        cmd += [
            "--noc_flows_file",
            resolve_vtr_source_file(config, noc_traffic, config.noc_traffic_dir),
        ]

    return includes, parse_cmd, second_parse_cmd, qor_parse_command, cmd


# pylint: disable=too-many-branches
def create_jobs(args, configs, after_run=False) -> List[Job]:
    """
    Create the jobs to be executed depending on the configs.
    """
    jobs = []
    for config in configs:
        # A task usually runs the CAD flow for a cartesian product of circuits and architectures.
        # NoC traffic flow files might need to be specified per circuit. If this is the case,
        # circuits and traffic flow files are paired. Otherwise, a cartesian product is performed
        # between circuits and traffic flow files. In both cases, the result is cartesian multiplied
        # with given architectures.
        if config.noc_traffic_list_type == "outer_product":
            combinations = list(itertools.product(config.circuits, config.noc_traffics))
        elif config.noc_traffic_list_type == "per_circuit":
            assert len(config.circuits) == len(config.noc_traffics)
            combinations = zip(config.circuits, config.noc_traffics)
        else:
            assert False, "Invalid noc_traffic_list_type"

        combinations = [
            (arch, circ, traffic_flow)
            for arch in config.archs
            for circ, traffic_flow in combinations
        ]

        for arch, circuit, noc_traffic in combinations:
            # If the circuit is constrained to only run on a specific arch, and
            # this arch is not that arch, skip this combination.
            circuit_arch_constraint = config.circuit_constraints[circuit]["arch"]
            if circuit_arch_constraint is not None and circuit_arch_constraint != arch:
                continue
            golden_results = load_parse_results(
                str(PurePath(config.config_dir).joinpath("golden_results.txt"))
            )
            abs_arch_filepath = resolve_vtr_source_file(config, arch, config.arch_dir)
            abs_circuit_filepath = resolve_vtr_source_file(config, circuit, config.circuit_dir)
            work_dir = get_work_dir_addr(arch, circuit, noc_traffic)

            run_dir = (
                str(Path(get_active_run_dir(find_task_dir(config, args.alt_tasks_dir))) / work_dir)
                if after_run
                else str(
                    Path(get_next_run_dir(find_task_dir(config, args.alt_tasks_dir))) / work_dir
                )
            )

            includes, parse_cmd, second_parse_cmd, qor_parse_command, cmd = create_cmd(
                abs_circuit_filepath, abs_arch_filepath, config, args, circuit, noc_traffic
            )

            if config.script_params_list_add:
                for value in config.script_params_list_add:
                    jobs.append(
                        create_job(
                            args,
                            config,
                            circuit,
                            includes,
                            arch,
                            noc_traffic,
                            value,
                            cmd,
                            parse_cmd,
                            second_parse_cmd,
                            qor_parse_command,
                            work_dir,
                            run_dir,
                            golden_results,
                        )
                    )
            else:
                jobs.append(
                    create_job(
                        args,
                        config,
                        circuit,
                        includes,
                        arch,
                        noc_traffic,
                        None,
                        cmd,
                        parse_cmd,
                        second_parse_cmd,
                        qor_parse_command,
                        work_dir,
                        run_dir,
                        golden_results,
                    )
                )

    return jobs


def create_job(
    args,
    config,
    circuit,
    include,
    arch,
    noc_flow,
    param,
    cmd,
    parse_cmd,
    second_parse_cmd,
    qor_parse_command,
    work_dir,
    run_dir,
    golden_results,
) -> Job:
    """
    Create an individual job with the specified parameters
    """
    param_string = "common" + (("_" + param.replace(" ", "_")) if param else "")

    # remove any address-related characters that might be in the param_string
    # To avoid creating invalid URL path
    path_str = "../"
    if path_str in param_string:
        param_string = param_string.replace("../", "")
        param_string = param_string.replace("-", "")
        circuit_2 = circuit.replace(".blif", "")
        if circuit_2 in param_string:
            ind = param_string.find(circuit_2)
            param_string = param_string[ind + len(circuit_2) + 1 :]

    for spec_char in [":", "<", ">", "|", "*", "?", "/", "."]:
        # replaced to create valid URL path
        param_string = param_string.replace(spec_char, "_")
    if not param:
        param = "common"

    expected_min_w = ret_expected_min_w(circuit, arch, golden_results, param)
    expected_min_w = (
        int(expected_min_w * args.minw_hint_factor)
        if hasattr(args, "minw_hint_factor")
        else expected_min_w
    )
    expected_min_w += expected_min_w % 2
    if expected_min_w > 0:
        cmd += ["--min_route_chan_width_hint", str(expected_min_w)]
    expected_vpr_status = ret_expected_vpr_status(arch, circuit, golden_results, param)
    if expected_vpr_status not in ("success", "Unknown"):
        cmd += ["-expect_fail", expected_vpr_status]
    current_parse_cmd = parse_cmd.copy()

    # Apply the command-line circuit constraints provided by the circuit
    # constraint list in the config file.
    apply_cmd_line_circuit_constraints(cmd, circuit, config)

    if config.parse_file:
        current_parse_cmd += [
            "arch={}".format(arch),
            "circuit={}".format(circuit),
            "noc_flow={}".format(noc_flow),
            "script_params={}".format(load_script_param(param)),
        ]
        current_parse_cmd.insert(0, run_dir + "/{}".format(load_script_param(param)))
    current_second_parse_cmd = second_parse_cmd.copy() if second_parse_cmd else None

    if config.second_parse_file:
        current_second_parse_cmd += [
            "arch={}".format(arch),
            "circuit={}".format(circuit),
            "noc_flow={}".format(noc_flow),
            "script_params={}".format(load_script_param(param)),
        ]
        current_second_parse_cmd.insert(0, run_dir + "/{}".format(load_script_param(param)))
    current_qor_parse_command = qor_parse_command.copy() if qor_parse_command else None

    if config.qor_parse_file:
        current_qor_parse_command += [
            "arch={}".format(arch),
            "circuit={}".format(circuit),
            "noc_flow={}".format(noc_flow),
            "script_params={}".format("common"),
        ]
        current_qor_parse_command.insert(0, run_dir + "/{}".format(load_script_param(param)))
    current_cmd = cmd.copy()
    current_cmd += ["-temp_dir", run_dir + "/{}".format(param_string)]

    if getattr(args, "use_previous", None):
        for prev_run, [extension, option] in args.use_previous:
            prev_run_dir = get_existing_run_dir(find_task_dir(config, args.alt_tasks_dir), prev_run)
            prev_work_path = Path(prev_run_dir) / work_dir / param_string
            prev_file = prev_work_path / "{}.{}".format(Path(circuit).stem, extension)
            if option == "REPLACE_BLIF":
                current_cmd[0] = str(prev_file)
                current_cmd += ["-start", "vpr"]
            else:
                current_cmd += [option, str(prev_file)]

    if param_string != "common":
        current_cmd += param.split(" ")

    return Job(
        config.task_name,
        arch,
        circuit,
        include,
        param_string,
        work_dir + "/" + param_string,
        current_cmd,
        current_parse_cmd,
        current_second_parse_cmd,
        current_qor_parse_command,
    )


# pylint: enable=too-many-arguments,too-many-locals


def ret_expected_min_w(circuit, arch, golden_results, script_params=None):
    """
    Retrive the expected minimum channel width from the golden results.
    """
    script_params = load_script_param(script_params)
    golden_metrics = golden_results.metrics(arch, circuit, script_params)
    if golden_metrics and "min_chan_width" in golden_metrics:
        return int(golden_metrics["min_chan_width"])
    return -1


def ret_expected_vpr_status(arch, circuit, golden_results, script_params=None):
    """
    Retrive the expected VPR status from the golden_results.
    """
    script_params = load_script_param(script_params)
    golden_metrics = golden_results.metrics(arch, circuit, script_params)
    if not golden_metrics or "vpr_status" not in golden_metrics:
        return "Unknown"

    return golden_metrics["vpr_status"]


def apply_cmd_line_circuit_constraints(cmd, circuit, config):
    """
    Apply the circuit constraints to the command line. If the circuit is not
    constrained for any key, this method will not do anything.
    """
    # Check if this circuit is constrained to a specific device.
    constrained_device = config.circuit_constraints[circuit]["device"]
    if constrained_device is not None:
        cmd += ["--device", constrained_device]
    # Check if the circuit has constrained atom locations.
    circuit_vpr_constraints = config.circuit_constraints[circuit]["constraints"]
    if circuit_vpr_constraints is not None:
        cmd += ["--read_vpr_constraints", circuit_vpr_constraints]
    # Check if the circuit has constrained route channel width.
    constrained_route_w = config.circuit_constraints[circuit]["route_chan_width"]
    if constrained_route_w is not None:
        cmd += ["--route_chan_width", constrained_route_w]


def resolve_vtr_source_file(config, filename, base_dir=""):
    """
    Resolves an filename with a base_dir

    Checks the following in order:
        1) filename as absolute path
        2) filename under config directory
        3) base_dir as absolute path (join filename with base_dir)
        4) filename and base_dir are relative paths (join under vtr_root)
    """

    # Absolute path
    if PurePath(filename).is_absolute():
        return filename

    # Under config
    config_path = Path(config.config_dir)
    assert config_path.is_absolute()
    joined_path = config_path / filename
    if joined_path.exists():
        return str(joined_path)

    # Under base dir
    base_path = Path(base_dir)
    if base_path.is_absolute():
        # Absolute base
        joined_path = base_path / filename
        if joined_path.exists():
            return str(joined_path)
    else:
        # Relative base under the VTR flow directory
        joined_path = paths.vtr_flow_path / base_dir / filename
        if joined_path.exists():
            return str(joined_path)

    # Not found
    raise InspectError("Failed to resolve VTR source file {}".format(filename))


def find_task_config_file(task_name):
    """
    See if we can find the config.txt assuming the task name is an
    absolute/relative path
    """

    base_dirs = []
    if PurePath(task_name).is_absolute():
        # Only check the root path since the path is aboslute
        base_dirs.append("/")
    else:
        # Not absolute path, so check from the current directory first
        base_dirs.append(".")

        vtr_flow_tasks_dir = str(paths.vtr_flow_path / "tasks")

        # Then the VTR tasks directory
        base_dirs.append(vtr_flow_tasks_dir)

    # Generate potential config files (from most to least specific)
    potential_config_file_paths = []
    for base_dir in base_dirs:
        # Assume points directly to a config.txt
        assume_config_path = str(PurePath(base_dir) / task_name)
        potential_config_file_paths.append(assume_config_path)

        # Assume points to a config dir (containing config.txt)
        assume_config_dir_path = str(PurePath(base_dir) / task_name / "config.txt")
        potential_config_file_paths.append(assume_config_dir_path)

        # Assume points to a task dir (containing config/config.txt)
        assume_task_dir_path = str(PurePath(base_dir) / task_name / "config" / "config.txt")
        potential_config_file_paths.append(assume_task_dir_path)

    # Find the first potential file that is valid
    for config_file in potential_config_file_paths:
        config_path = Path(config_file)
        is_file = config_path.is_file()
        is_named_config = config_path.name == "config.txt"
        is_in_config_dir = config_path.parent.name == "config"

        if is_file and is_named_config and is_in_config_dir:
            return config_path.resolve()

    raise VtrError("Could not find config/config.txt for task {name}".format(name=task_name))
