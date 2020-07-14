import re
from collections import OrderedDict

try:
    #Try for the fast c-based version first
    import xml.etree.cElementTree as ET
except ImportError:
    #Fall back on python implementation
    import xml.etree.ElementTree as ET

from vtr import load_config_lines
from vtr.error import InspectError

class ParsePattern:
    def __init__(self, name, filename, regex_str, default_value=None):
        self._name = name
        self._filename = filename
        self._regex = re.compile(regex_str)
        self._default_value = default_value

    def name(self):
        return self._name

    def filename(self):
        return self._filename

    def regex(self):
        return self._regex

    def default_value(self):
        return self._default_value

class PassRequirement(object):
    def __init__(self, metric):
        self._metric = metric
        self._type = type

    def metric(self):
        return self._metric

    def type(self):
        raise NotImplementedError()

    def check_passed(golden_value, check_value):
        raise NotImplementedError()

class EqualPassRequirement(PassRequirement):
    def __init__(self, metric):
        super(EqualPassRequirement, self).__init__(metric)

    def type(self):
        return "Equal"

    def check_passed(self, golden_value, check_value):
        if golden_value == check_value:
            return True, ""
        else:
            return False, "Task value '{}' does not match golden value '{}'".format(golden_value, check_value)

class RangePassRequirement(PassRequirement):

    def __init__(self, metric, min_value=None, max_value=None):
        super(RangePassRequirement, self).__init__(metric)

        if max_value < min_value:
            raise InspectError("Invalid range specification (max value larger than min value)")

        self._min_value = min_value
        self._max_value = max_value

    def type(self):
        return "Range"

    def min_value(self):
        return self._min_value

    def max_value(self):
        return self._max_value

    def check_passed(self, golden_value, check_value):
        if golden_value == None and check_value == None:
            return True, "both golden and check are None"
        elif golden_value == None and check_value != None:
            return False, "golden value is None, but check value is {}".format(check_value)
        elif golden_value != None and check_value == None:
            return False, "golden value is {}, but check value is None".format(golden_value)

        assert golden_value != None
        assert check_value != None

        try:
            golden_value = float(golden_value)
        except ValueError as e:
            raise InspectError("Failed to convert golden value '{}' to float".format(golden_value))

        try:
            check_value = float(check_value)
        except ValueError as e:
            raise InspectError("Failed to convert check value '{}' to float".format(check_value))

        if golden_value == 0.: #Avoid division by zero

            if golden_value == check_value:
                return True, "golden and check both equal 0"
            else:
                return False, "unable to normalize relative value (golden value is zero)".format(norm_check_value, min_value(), max_value())

        else:
            norm_check_value = check_value / golden_value

            if self.min_value() <= norm_check_value <= self.max_value():
                return True, "relative value within range"
            else:
                return False, "relative value {} outside of range [{},{}]".format(norm_check_value, min_value(), max_value())

class ParseResults:
    def __init__(self):
        self._metrics = OrderedDict()

    def primary_keys(self):
        return ("architecture", "circuit")

    def add_result(self, arch, circuit, parse_result):
        self._metrics[(arch, circuit)] = parse_result

    def metrics(self, arch, circuit):
        if (arch, circuit) in self._metrics:
            return self._metrics[(arch, circuit)]
        else:
            return None

    def all_metrics(self):
        return self._metrics

def determine_lut_size(architecture_file):
    """
    Determines the maximum LUT size (K) in an architecture file.

    Assumes LUTs are represented as BLIF '.names'
    """
    arch_xml = ET.parse(architecture_file).getroot()

    lut_size = 0
    saw_blif_names = False
    for elem in arch_xml.findall('.//pb_type'): #Xpath recrusive search for 'pb_type'
        blif_model = elem.get('blif_model')
        if blif_model and blif_model == ".names":
            saw_blif_names = True
            input_port = elem.find('input')

            input_width = int(input_port.get('num_pins'))
            assert input_width > 0

            #Keep the maximum lut size found (i.e. fracturable architectures)
            lut_size = max(lut_size, input_width)

    if saw_blif_names and lut_size == 0:
        raise InspectError(msg="Could not identify valid LUT size (K)",
                           filename=architecture_file)

    return lut_size

def determine_memory_addr_width(architecture_file):
    """
    Determines the maximum RAM block address width in an architecture file

    Assumes RAMS are represented using the standard VTR primitives (.subckt single_port_ram, .subckt dual_port_ram etc.)
    """
    arch_xml = ET.parse(architecture_file).getroot()

    mem_addr_width = 0
    saw_ram = False
    for elem in arch_xml.findall('.//pb_type'): #XPATH for recursive search
        blif_model = elem.get('blif_model')
        if blif_model and "port_ram" in blif_model:
            saw_ram = True
            for input_port in elem.findall('input'):
                port_name = input_port.get('name')
                if "addr" in port_name:
                    input_width = int(input_port.get('num_pins'))
                    mem_addr_width = max(mem_addr_width, input_width)

    if saw_ram and mem_addr_width == 0:
        raise InspectError(msg="Could not identify RAM block address width",
                           filename=architecture_file)

    return mem_addr_width

def determine_min_W(log_filename):
    min_W_regex = re.compile(r"\s*Best routing used a channel width factor of (?P<min_W>\d+).")
    with open(log_filename) as f:
        for line in f:
            match = min_W_regex.match(line)
            if match:
                return int(match.group("min_W"))

    raise InspectError(msg="Failed to find minimum channel width.",
                      filename=log_filename)
