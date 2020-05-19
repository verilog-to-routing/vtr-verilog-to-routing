from pathlib import Path
from pathlib import PurePath

from vtr import VtrError, find_vtr_root, load_list_file

class TaskConfig:
    """
    An object representing a task config file
    """
    def __init__(self,
                 task_name,
                 config_dir,
                 circuits_dir,
                 archs_dir,
                 circuit_list_add,
                 arch_list_add,
                 parse_file,
                 script_path=None,
                 script_params=None,
                 pass_requirements_file=None,
                 sdc_dir=None,
                 qor_parse_file=None,
                 cmos_tech_behavior=None,
                 pad_file=None):
        self.task_name = task_name
        self.config_dir = config_dir
        self.circuit_dir = circuits_dir
        self.arch_dir = archs_dir
        self.circuits = circuit_list_add
        self.archs = arch_list_add
        self.parse_file = parse_file
        self.script_path = script_path
        self.script_params = script_params
        self.pass_requirements_file = pass_requirements_file
        self.sdc_dir = sdc_dir
        self.qor_parse_file = qor_parse_file
        self.cmos_tech_behavior = cmos_tech_behavior
        self.pad_file = pad_file

def load_task_config(config_file):
    #Load the file stripping comments
    values = load_list_file(config_file)

    #Keys that can appear only once
    unique_keys = set(["circuits_dir",
                      "archs_dir",
                      "parse_file",
                      "script_path",
                      "script_params",
                      "pass_requirements_file",
                      "sdc_dir",
                      "qor_parse_file",
                      "cmos_tech_behavior",
                      "pad_file"])

    #Keys that are repeated to create lists
    repeated_keys = set(["circuit_list_add",
                        "arch_list_add"])

    #Keys that are required
    required_keys = set(["circuits_dir", 
                         "archs_dir", 
                         "circuit_list_add", 
                         "arch_list_add", 
                         "parse_file"])

    #Interpret the file
    key_values = {}
    for line in values:
        key, value = line.split('=')

        #Trim whitespace
        key = key.strip()
        value = value.strip()

        if key in unique_keys:
            if key not in key_values:
                key_values[key] = value
            else:
                raise VtrError("Duplicate {key} in config file {file}".format(key=key, file=config_file))

        elif key in repeated_keys:
            if key not in key_values:
                key_values[key] = []
            key_values[key].append(value)

        else:
            #All valid keys should have been collected by now
            raise VtrError("Unrecognzied key '{key}' in config file {file}".format(key=key, file=config_file))

    #We split the script params into a list
    if 'script_params' in key_values:
        key_values['script_params'] = key_values['script_params'].split(' ')

    #Check that all required fields were specified
    for required_key in required_keys:
        if required_key not in key_values:
            raise VtrError("Missing required key '{key}' in config file {file}".format(key=required_key, file=config_file))

    #Useful meta-data about the config
    config_dir = str(Path(config_file).parent)
    key_values['config_dir'] = config_dir
    key_values['task_name'] = Path(config_dir).parent.name

    #Create the task config object
    return TaskConfig(**key_values)


def find_task_config_file(task_name):
    #
    # See if we can find the config.txt assuming the task name is an 
    # absolute/relative path
    #

    base_dirs = []
    if PurePath(task_name).is_absolute:
        #Only check the root path since the path is aboslute
        base_dirs.append('/')
    else:
        #Not absolute path, so check from the current directory first
        base_dirs.append('.')

        vtr_root = find_vtr_root()
        vtr_flow_tasks_dir = str(PurePath(vtr_root) / "vtr_flow" / "tasks")

        #Then the VTR tasks directory
        base_dirs.append(vtr_flow_tasks_dir)

    #Generate potential config files (from most to least specific)
    potential_config_file_paths = []
    for base_dir in base_dirs:
        #Assume points directly to a config.txt
        assume_config_path = str(PurePath(base_dir) / task_name)
        potential_config_file_paths.append(assume_config_path)

        #Assume points to a config dir (containing config.txt)
        assume_config_dir_path = str(PurePath(base_dir) / task_name / "config.txt")
        potential_config_file_paths.append(assume_config_dir_path)

        #Assume points to a task dir (containing config/config.txt)
        assume_task_dir_path = str(PurePath(base_dir) / task_name / "config" / "config.txt")
        potential_config_file_paths.append(assume_task_dir_path)

    #Find the first potential file that is valid
    for config_file in potential_config_file_paths:
        config_path = Path(config_file)
        is_file = config_path.is_file()
        is_named_config = config_path.name == "config.txt"
        is_in_config_dir = config_path.parent.name == "config"

        if is_file and is_named_config and is_in_config_dir:
            return config_path.resolve()

    raise VtrError("Could not find config/config.txt for task {name}".format(name=task_name))

