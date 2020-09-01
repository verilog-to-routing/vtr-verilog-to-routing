"""
    module that contains functions to inspect various files to determine important values
"""
import re

try:
    # Try for the fast c-based version first
    import xml.etree.cElementTree as ET
except ImportError:
    # Fall back on python implementation
    import xml.etree.ElementTree as ET

from vtr.error import InspectError


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
