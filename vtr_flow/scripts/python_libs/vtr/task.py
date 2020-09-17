"""
Module that contains the task functions
"""
from pathlib import Path
from pathlib import PurePath
from shlex import split
import itertools

from vtr import (
    VtrError,
    InspectError,
    load_list_file,
    load_parse_results,
    get_next_run_dir,
    find_task_dir,
    load_script_param,
    get_latest_run_dir,
    paths,
)

# pylint: disable=too-many-instance-attributes, too-many-arguments, too-many-locals,too-few-public-methods
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
        second_parse_file=None,
        script_path=None,
        script_params=None,
        script_params_common=None,
        script_params_list_add=None,
        pass_requirements_file=None,
        sdc_dir=None,
        qor_parse_file=None,
        cmos_tech_behavior=None,
        pad_file=None,
    ):
        self.task_name = task_name
        self.config_dir = config_dir
        self.circuit_dir = circuits_dir
        self.arch_dir = archs_dir
        self.circuits = circuit_list_add
        self.archs = arch_list_add
        self.parse_file = parse_file
        self.second_parse_file = second_parse_file
        self.script_path = script_path
        self.script_params = script_params
        self.script_params_common = script_params_common
        self.script_params_list_add = script_params_list_add
        self.pass_requirements_file = pass_requirements_file
        self.sdc_dir = sdc_dir
        self.qor_parse_file = qor_parse_file
        self.cmos_tech_behavior = cmos_tech_behavior
        self.pad_file = pad_file


# pylint: enable=too-few-public-methods


class Job:
    """
    A class to store the nessesary information for a job that needs to be run.
    """

    def __init__(
        self,
        task_name,
        arch,
        circuit,
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

    def work_dir(self, run_dir):
        """
        return the work directory of the job
        """
        return str(PurePath(run_dir).joinpath(self._work_dir))


# pylint: enable=too-many-instance-attributes


def load_task_config(config_file):
    """
    Load task config information
    """
    # Load the file stripping comments
    values = load_list_file(config_file)

    # Keys that can appear only once
    unique_keys = set(
        [
            "circuits_dir",
            "archs_dir",
            "parse_file",
            "script_path",
            "script_params",
            "script_params_common",
            "pass_requirements_file",
            "sdc_dir",
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
        key, value = line.split("=")

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
                "Unrecognzied key '{key}' in config file {file}".format(key=key, file=config_file)
            )

    # We split the script params into a list
    if "script_params" in key_values:
        key_values["script_params"] = split(key_values["script_params"])

    if "script_params_common" in key_values:
        key_values["script_params_common"] = split(key_values["script_params_common"])

    check_required_feilds(config_file, required_keys, key_values)

    # Useful meta-data about the config
    config_dir = str(Path(config_file).parent)
    key_values["config_dir"] = config_dir
    key_values["task_name"] = Path(config_dir).parent.name

    # Create the task config object
    return TaskConfig(**key_values)


def check_required_feilds(config_file, required_keys, key_values):
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


def create_jobs(args, configs, longest_name=0, longest_arch_circuit=0, after_run=False):
    """
    Create the jobs to be executed depending on the configs.
    """
    jobs = []
    for config in configs:
        for arch, circuit in itertools.product(config.archs, config.circuits):
            golden_results = load_parse_results(
                str(PurePath(config.config_dir).joinpath("golden_results.txt"))
            )
            abs_arch_filepath = resolve_vtr_source_file(config, arch, config.arch_dir)
            abs_circuit_filepath = resolve_vtr_source_file(config, circuit, config.circuit_dir)
            work_dir = str(PurePath(arch).joinpath(circuit))

            run_dir = (
                str(Path(get_latest_run_dir(find_task_dir(config))) / work_dir)
                if after_run
                else str(Path(get_next_run_dir(find_task_dir(config))) / work_dir)
            )

            # Collect any extra script params from the config file
            cmd = [abs_circuit_filepath, abs_arch_filepath]

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
                ["--fix_pins", resolve_vtr_source_file(config, config.pad_file)]
                if config.pad_file
                else []
            )

            if config.sdc_dir:
                cmd += [
                    "-sdc_file",
                    "{}/{}.sdc".format(config.sdc_dir, Path(circuit).stem),
                ]

            parse_cmd = None
            second_parse_cmd = None
            qor_parse_command = None
            if config.parse_file:
                parse_cmd = [
                    resolve_vtr_source_file(
                        config, config.parse_file, str(PurePath("parse").joinpath("parse_config")),
                    )
                ]

            if config.second_parse_file:
                second_parse_cmd = [
                    resolve_vtr_source_file(
                        config,
                        config.second_parse_file,
                        str(PurePath("parse").joinpath("parse_config")),
                    )
                ]

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
            if config.script_params_list_add:
                for value in config.script_params_list_add:
                    jobs.append(
                        create_job(
                            args,
                            config,
                            circuit,
                            arch,
                            value,
                            cmd,
                            parse_cmd,
                            second_parse_cmd,
                            qor_parse_command,
                            work_dir,
                            run_dir,
                            longest_name,
                            longest_arch_circuit,
                            golden_results,
                        )
                    )
            else:
                jobs.append(
                    create_job(
                        args,
                        config,
                        circuit,
                        arch,
                        None,
                        cmd,
                        parse_cmd,
                        second_parse_cmd,
                        qor_parse_command,
                        work_dir,
                        run_dir,
                        longest_name,
                        longest_arch_circuit,
                        golden_results,
                    )
                )

    return jobs


def create_job(
    args,
    config,
    circuit,
    arch,
    param,
    cmd,
    parse_cmd,
    second_parse_cmd,
    qor_parse_command,
    work_dir,
    run_dir,
    longest_name,
    longest_arch_circuit,
    golden_results,
):
    """
    Create an individual job with the specified parameters
    """
    param_string = "common" + (("_" + param.replace(" ", "_")) if param else "")
    if not param:
        param = "common"
    # determine spacing for nice output
    num_spaces_before = int((longest_name - len(config.task_name))) + 8
    num_spaces_after = int((longest_arch_circuit - len(work_dir + "/{}".format(param_string))))
    cmd += [
        "-name",
        "{}:{}{}/{}{}".format(
            config.task_name,
            " " * num_spaces_before,
            work_dir,
            param_string,
            " " * num_spaces_after,
        ),
    ]

    cmd += ["-temp_dir", run_dir + "/{}".format(param_string)]
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

    if config.parse_file:
        current_parse_cmd += [
            "arch={}".format(arch),
            "circuit={}".format(circuit),
            "script_params={}".format(load_script_param(param)),
        ]
        current_parse_cmd.insert(0, run_dir + "/{}".format(load_script_param(param)))
    current_second_parse_cmd = second_parse_cmd.copy() if second_parse_cmd else None

    if config.second_parse_file:
        current_second_parse_cmd += [
            "arch={}".format(arch),
            "circuit={}".format(circuit),
            "script_params={}".format(load_script_param(param)),
        ]
        current_second_parse_cmd.insert(0, run_dir + "/{}".format(load_script_param(param)))
    current_qor_parse_command = qor_parse_command.copy() if qor_parse_command else None

    if config.qor_parse_file:
        current_qor_parse_command += [
            "arch={}".format(arch),
            "circuit={}".format(circuit),
            "script_params={}".format("common"),
        ]
        current_qor_parse_command.insert(0, run_dir + "/{}".format(load_script_param(param)))
    current_cmd = cmd.copy()
    if param_string != "common":
        current_cmd += param.split(" ")
    return Job(
        config.task_name,
        arch,
        circuit,
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
