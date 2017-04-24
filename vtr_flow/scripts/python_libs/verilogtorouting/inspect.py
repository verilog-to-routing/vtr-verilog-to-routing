import re
from collections import OrderedDict

try:
    #Try for the fast c-based version first
    import xml.etree.cElementTree as ET
except ImportError:
    #Fall back on python implementation
    import xml.etree.ElementTree as ET

from util import load_config_lines
from error import InspectError

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

def load_parse_patterns(parse_config_filepath):
    parse_patterns = OrderedDict()

    for line in load_config_lines(parse_config_filepath):

        components = line.split(';')

    
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
                raise InspectError("Duplicate parse pattern name '{}'".format(name), parse_config_filepath)

        else:
            raise InspectError("Invalid parse format line: '{}'".format(line), parse_config_filepath)

    return parse_patterns

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
