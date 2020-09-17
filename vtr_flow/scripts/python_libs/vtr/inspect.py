"""
    module that contains functions to inspect various files to determine important values
"""
import re
from collections import OrderedDict
from pathlib import Path
import abc

try:
    # Try for the fast c-based version first
    import xml.etree.cElementTree as ET
except ImportError:
    # Fall back on python implementation
    import xml.etree.ElementTree as ET

from vtr.error import InspectError
from vtr import load_config_lines


class ParsePattern:
    """ Pattern for parsing log files """

    def __init__(self, name, filename, regex_str, default_value=None):
        self._name = name
        self._filename = filename
        self._regex = re.compile(regex_str)
        self._default_value = default_value

    def name(self):
        """ Return name of what is being parsed for """
        return self._name

    def filename(self):
        """ Log filename to parse """
        return self._filename

    def regex(self):
        """ Regex expression to use for parsing """
        return self._regex

    def default_value(self):
        """ Return the default parse value """
        return self._default_value


class PassRequirement(abc.ABC):
    """ Used to check if a parsed value passes an expression """

    def __init__(self, metric):
        self._metric = metric
        self._type = type

    def metric(self):
        """ Return pass matric """
        return self._metric

    @abc.abstractmethod
    def type(self):
        """ Return the type of requirement checking """

    @abc.abstractmethod
    def check_passed(self, golden_value, check_value, check_string="golden value"):
        """ Return whether the check passed """


class EqualPassRequirement(PassRequirement):
    """ Used to check if parsed value is equal to golden value """

    def type(self):
        return "Equal"

    def check_passed(self, golden_value, check_value, check_string="golden value"):
        if golden_value == check_value:
            return True, ""

        return (
            False,
            "Task value '{}' does not match {} '{}'".format(
                check_value, check_string, golden_value
            ),
        )


class RangePassRequirement(PassRequirement):
    """ Used to check if parsed value is within a range """

    def __init__(self, metric, min_value=None, max_value=None):
        super().__init__(metric)

        if max_value < min_value:
            raise InspectError("Invalid range specification (max value larger than min value)")

        self._min_value = min_value
        self._max_value = max_value

    def type(self):
        return "Range"

    def min_value(self):
        """ Get min value of golden range """
        return self._min_value

    def max_value(self):
        """ Get max value of golden range """
        return self._max_value

    def check_passed(self, golden_value, check_value, check_string="golden value"):
        """ Check if parsed value is within a range or equal to golden value """

        if golden_value is None or check_value is None:
            if golden_value is None and check_value is None:
                ret_value = True
                ret_str = "both golden and check are None"
            elif check_value is None:
                ret_value = False
                ret_str = ("{} is {}, but check value is None".format(check_string, golden_value),)
            else:
                ret_value = False
                ret_str = "{} is None, but check value is {}".format(check_string, check_value)
            return (ret_value, ret_str)

        assert golden_value is not None
        assert check_value is not None

        original_golden_value = golden_value
        try:
            golden_value = float(golden_value)
        except ValueError:
            raise InspectError(
                "Failed to convert {} '{}' to float".format(check_string, golden_value)
            ) from ValueError
        original_check_value = check_value
        try:
            check_value = float(check_value)
        except ValueError:
            raise InspectError(
                "Failed to convert check value '{}' to float".format(check_value)
            ) from ValueError

        norm_check_value = None
        if golden_value == 0.0:  # Avoid division by zero
            if golden_value == check_value:
                return True, "{} and check both equal 0".format(check_string)
            norm_check_value = float("inf")
        else:
            norm_check_value = check_value / golden_value

        if original_check_value == original_golden_value:
            return True, "Check value equal to {}".format(check_string)

        if self.min_value() <= norm_check_value <= self.max_value():
            return True, "relative value within range"

        return (
            False,
            "relative value {} outside of range [{},{}] "
            "and not equal to {} value: {}".format(
                norm_check_value, self.min_value(), self.max_value(), check_string, golden_value,
            ),
        )


class RangeAbsPassRequirement(PassRequirement):
    """ Check if value is in some relative ratio range, or below some absolute value """

    def __init__(self, metric, min_value=None, max_value=None, abs_threshold=None):
        super().__init__(metric)

        if max_value < min_value:
            raise InspectError("Invalid range specification (max value larger than min value)")

        self._min_value = min_value
        self._max_value = max_value
        self._abs_threshold = abs_threshold

    def type(self):
        """ Return requirement type """
        return "Range"

    def min_value(self):
        """ Return min value of ratio range """
        return self._min_value

    def max_value(self):
        """ Return max value of ratio range """
        return self._max_value

    def abs_threshold(self):
        """ Get absolute threshold """
        return self._abs_threshold

    def check_passed(self, golden_value, check_value, check_string="golden value"):
        """
        Check if parsed value is within acceptable range,
        absolute value or equal to golden value
        """

        # Check for nulls
        if golden_value is None or check_value is None:
            if golden_value is None and check_value is None:
                ret_value = True
                ret_str = "both {} and check are None".format(check_string)
            elif golden_value is None:
                ret_value = False
                ret_str = "{} is None, but check value is {}".format(check_string, check_value)
            else:
                ret_value = False
                ret_str = "{} is {}, but check value is None".format(check_string, golden_value)

            return (ret_value, ret_str)

        assert golden_value is not None
        assert check_value is not None

        # Convert values to float
        original_golden_value = golden_value
        try:
            golden_value = float(golden_value)
        except ValueError:
            raise InspectError(
                "Failed to convert {} '{}' to float".format(check_string, golden_value)
            ) from ValueError

        original_check_value = check_value
        try:
            check_value = float(check_value)
        except ValueError:
            raise InspectError(
                "Failed to convert check value '{}' to float".format(check_value)
            ) from ValueError

        # Get relative ratio
        norm_check_value = None
        if golden_value == 0.0:  # Avoid division by zero
            if golden_value == check_value:
                return True, "{} and check both equal 0".format(check_string)
            norm_check_value = float("inf")
        else:
            norm_check_value = check_value / golden_value

        # Check if the original values match
        if original_check_value == original_golden_value:
            return True, "Check value equal to {}".format(check_string)

        # Check if value within range
        if (self.min_value() <= norm_check_value <= self.max_value()) or abs(
            check_value - golden_value
        ) <= self.abs_threshold():
            return True, "relative value within range"

        return (
            False,
            "relative value {} outside of range [{},{}], "
            "above absolute threshold {} and not equal to {} value: {}".format(
                norm_check_value,
                self.min_value(),
                self.max_value(),
                self.abs_threshold(),
                check_string,
                golden_value,
            ),
        )


class ParseResults:
    """ This class contains all parse results and metrics """

    PRIMARY_KEYS = ("architecture", "circuit", "script_params")

    def __init__(self):
        self._metrics = OrderedDict()

    def add_result(self, arch, circuit, parse_result, script_param=None):
        """ Add new parse result for given arch/circuit pair """
        script_param = load_script_param(script_param)
        self._metrics[(arch, circuit, script_param)] = parse_result

    def metrics(self, arch, circuit, script_param=None):
        """ Return individual metric based on the architechure, circuit and script"""
        script_param = load_script_param(script_param)
        if (arch, circuit, script_param) in self._metrics:
            return self._metrics[(arch, circuit, script_param)]
        return None

    def all_metrics(self):
        """ Return all metric results """
        return self._metrics


def load_script_param(script_param):
    """
    Create script parameter string to be used in task names and output.
    """
    if script_param and "common" not in script_param:
        script_param = "common_" + script_param
    if script_param:
        script_param = script_param.replace(" ", "_")
    else:
        script_param = "common"
    return script_param


def load_parse_patterns(parse_config_filepath):
    """
    Loads the parse patterns from the desired file.
    These parse patterns are later used to load in the results file
    The lines of this file should be formated in either of the following ways:
        name;path;regex;[default value]
        name;path;regex
    """
    parse_patterns = OrderedDict()

    for line in load_config_lines(parse_config_filepath):

        components = line.split(";")

        if len(components) == 3 or len(components) == 4:

            name = components[0]
            filepath = components[1]
            regex_str = components[2]

            default_value = None
            if len(components) == 4:
                default_value = components[3]

            if name not in parse_patterns:
                parse_patterns[name] = ParsePattern(name, filepath, regex_str, default_value)
            else:
                raise InspectError(
                    "Duplicate parse pattern name '{}'".format(name), parse_config_filepath,
                )

        else:
            raise InspectError(
                "Invalid parse format line: '{}'".format(line), parse_config_filepath
            )

    return parse_patterns


def load_pass_requirements(pass_requirements_filepath):
    """
    Load the pass requirements from particular file
    The lines of the pass requiremtents file should follow one of the following format:
        name;Range(min,max)
        name;RangeAbs(min,max,absolute_value)
        name;Equal()
    """
    parse_patterns = OrderedDict()

    for line in load_config_lines(pass_requirements_filepath):
        components = line.split(";")

        if len(components) != 2:
            raise InspectError(
                "Invalid pass requirement format line: '{}'".format(line),
                pass_requirements_filepath,
            )

        metric = components[0]
        expr = components[1]

        if metric in parse_patterns:
            raise InspectError(
                "Duplicate pass requirement for '{}'".format(metric), pass_requirements_filepath,
            )

        func, params_str = expr.split("(")
        params_str = params_str.rstrip(")")
        params = []

        if params_str != "":
            params = params_str.split(",")

        if func == "Range":
            if len(params) != 2:
                raise InspectError(
                    "Range() pass requirement function requires two arguments",
                    pass_requirements_filepath,
                )

            parse_patterns[metric] = RangePassRequirement(
                metric, float(params[0]), float(params[1])
            )
        elif func == "RangeAbs":
            if len(params) != 3:
                raise InspectError(
                    "RangeAbs() pass requirement function requires two arguments",
                    pass_requirements_filepath,
                )

            parse_patterns[metric] = RangeAbsPassRequirement(
                metric, float(params[0]), float(params[1]), float(params[2])
            )
        elif func == "Equal":
            if len(params) != 0:
                raise InspectError(
                    "Equal() pass requirement function requires no arguments",
                    pass_requirements_filepath,
                )
            parse_patterns[metric] = EqualPassRequirement(metric)
        else:
            raise InspectError(
                "Unexpected pass requirement function '{}' for metric '{}'".format(func, metric),
                pass_requirements_filepath,
            )

    return parse_patterns


def load_parse_results(parse_results_filepath):
    """
    Load the results from the parsed result file.
    """
    header = []

    parse_results = ParseResults()
    if not Path(parse_results_filepath).exists():
        return parse_results

    with open(parse_results_filepath) as file:
        for lineno, row in enumerate(file):
            if row[0] == "+":
                row = row[1:]
            elements = [elem.strip() for elem in row.split("\t")]
            if lineno == 0:
                header = elements
            else:
                result = OrderedDict()

                arch = None
                circuit = None
                script_param = None
                for i, elem in enumerate(elements):
                    metric = header[i]

                    if elem == "":
                        elem = None

                    if metric == "arch":
                        metric = "architecture"

                    if metric == "architecture":
                        arch = elem
                    elif metric == "circuit":
                        circuit = elem
                    elif metric == "script_params":
                        script_param = elem

                    result[metric] = elem

                if not (arch and circuit):
                    print(parse_results_filepath)

                parse_results.add_result(arch, circuit, result, script_param)

    return parse_results


def determine_lut_size(architecture_file):
    """
    Determines the maximum LUT size (K) in an architecture file.

    Assumes LUTs are represented as BLIF '.names'
    """
    arch_xml = ET.parse(architecture_file).getroot()

    lut_size = 0
    saw_blif_names = False
    for elem in arch_xml.findall(".//pb_type"):  # Xpath recrusive search for 'pb_type'
        blif_model = elem.get("blif_model")
        if blif_model and blif_model == ".names":
            saw_blif_names = True
            input_port = elem.find("input")

            input_width = int(input_port.get("num_pins"))
            assert input_width > 0

            # Keep the maximum lut size found (i.e. fracturable architectures)
            lut_size = max(lut_size, input_width)

    if saw_blif_names and lut_size == 0:
        raise InspectError("Could not identify valid LUT size (K)", filename=architecture_file)

    return lut_size


def determine_memory_addr_width(architecture_file):
    """
    Determines the maximum RAM block address width in an architecture file

    Assumes RAMS are represented using the standard VTR primitives
    (.subckt single_port_ram, .subckt dual_port_ram etc.)
    """
    arch_xml = ET.parse(architecture_file).getroot()

    mem_addr_width = 0
    saw_ram = False
    for elem in arch_xml.findall(".//pb_type"):  # XPATH for recursive search
        blif_model = elem.get("blif_model")
        if blif_model and "port_ram" in blif_model:
            saw_ram = True
            for input_port in elem.findall("input"):
                port_name = input_port.get("name")
                if "addr" in port_name:
                    input_width = int(input_port.get("num_pins"))
                    mem_addr_width = max(mem_addr_width, input_width)

    if saw_ram and mem_addr_width == 0:
        raise InspectError("Could not identify RAM block address width", filename=architecture_file)

    return mem_addr_width


def determine_min_w(log_filename):
    """
        determines the miniumum width.
    """
    min_w_regex = re.compile(r"\s*Best routing used a channel width factor of (?P<min_w>\d+).")
    with open(log_filename) as file:
        for line in file:
            match = min_w_regex.match(line)
            if match:
                return int(match.group("min_w"))

    raise InspectError("Failed to find minimum channel width.", filename=log_filename)
