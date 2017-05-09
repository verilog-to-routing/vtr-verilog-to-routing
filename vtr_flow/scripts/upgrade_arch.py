#!/usr/bin/env python

import argparse
import sys
from collections import OrderedDict

from lxml import etree as ET

class SequentialPort:
    def __init__(self, port, clock):
        self.port = port
        self.clock = clock

class CombEdge:
    def __init__(self, src_port, sink_port):
        self.src_port = src_port
        self.sink_ports = sink_port

class ModelTiming:
    def __init__(self):
        self.sequential_ports = set()
        self.comb_edges = {}

#To add support for new upgrades/features add the identifier string here
#and added a guarded call in main()
supported_upgrades = [
    "add_model_timing",
    "upgrade_fc_overrides"
]

def parse_args():
    parser = argparse.ArgumentParser(description="Upgrade Legacy VTR architecture files")

    parser.add_argument("xml_file", help="xml_file to update (modified in place)")
    parser.add_argument("--features",
                        nargs="*",
                        help="List of features/upgrades to apply (default: %(default)s)",
                        choices=supported_upgrades,
                        default=supported_upgrades)
    parser.add_argument("--debug", default=False, action="store_true", help="Print to stdout instead of modifying file inplace")
    parser.add_argument("--pretty", default=False, action="store_true", help="Pretty print the output?")

    return parser.parse_args()

def main():
    args = parse_args()

    print args.xml_file
    parser = ET.XMLParser()
    root = ET.parse(args.xml_file, parser)

    root_tags = root.findall(".")
    assert len(root_tags) == 1
    arch = root_tags[0]
    assert arch.tag == "architecture"

    modified = False
    if "add_model_timing" in args.features:
        modified = modified or add_model_timing(arch)

    if "upgrade_fc_overrides" in args.features:
        modified = modified or upgrade_fc_overrides(arch)

    if modified:
        if args.debug:
            root.write(sys.stdout, pretty_print=args.pretty)
        else:
            with open(args.xml_file, "w") as f:
                root.write(f, pretty_print=args.pretty)

def add_model_timing(arch):
    models = arch.findall("./models/model")

    #Find all primitive pb types
    prim_pbs = arch.findall(".//pb_type[@blif_model]")

    #Build up the timing specifications from 
    default_models = frozenset([".input", ".output", ".latch", ".names"])
    primitive_timing_specs = {}
    for prim_pb in prim_pbs:

        blif_model = prim_pb.attrib['blif_model']

        if blif_model in default_models:
            continue

        assert blif_model.startswith(".subckt ")
        blif_model = blif_model[len(".subckt "):]

        if blif_model not in primitive_timing_specs:
            primitive_timing_specs[blif_model] = ModelTiming()

        #Find combinational edges
        for delay_const in prim_pb.findall("./delay_constant"):
            iports = get_port_names(delay_const.attrib['in_port'])
            oports = get_port_names(delay_const.attrib['out_port'])

            for iport in iports:
                if iport not in primitive_timing_specs[blif_model].comb_edges:
                    primitive_timing_specs[blif_model].comb_edges[iport] = set()

                for oport in oports:
                    primitive_timing_specs[blif_model].comb_edges[iport].add(oport)

        #Find sequential ports
        for xpath_pattern in ["./T_setup", "./T_clock_to_Q"]:
            for seq_tag in prim_pb.findall(xpath_pattern):
                ports = get_port_names(seq_tag.attrib['port'])
                for port in ports:
                    clk = seq_tag.attrib['clock']

                    primitive_timing_specs[blif_model].sequential_ports.add((port, clk))

    changed = False
    for model in models:
        if model.attrib['name'] not in primitive_timing_specs:
            print "Warning: no timing specifications found for {}".format(mode.attrib['name'])
            continue

        model_timing = primitive_timing_specs[model.attrib['name']]

        #Mark model ports as sequential
        for port_name, clock_name in model_timing.sequential_ports:
            port = model.find(".//port[@name='{}']".format(port_name))
            port.attrib['clock'] = clock_name
            changed = True

        #Mark combinational edges from sources to sink ports
        for src_port, sink_ports in model_timing.comb_edges.iteritems():
            xpath_pattern = "./input_ports/port[@name='{}']".format(src_port)
            port = model.find(xpath_pattern)
            port.attrib['combinational_sink_ports'] = ' '.join(sink_ports)
            changed = True

    return changed

def upgrade_fc_overrides(arch):
    fc_tags = arch.findall(".//fc")

    changed = False
    for fc_tag in fc_tags:
        assert fc_tag.tag == "fc"

        old_fc_pin_overrides = fc_tag.findall("./pin")
        old_fc_seg_overrides = fc_tag.findall("./segment")

        assert len(old_fc_pin_overrides) == 0 or len(old_fc_seg_overrides) == 0, "Can only have pin or seg overrides (not both)"

        for old_pin_override in old_fc_pin_overrides:

            port = old_pin_override.attrib['name']
            fc_type = old_pin_override.attrib['fc_type']
            fc_val = old_pin_override.attrib['fc_val']
            
            fc_tag.remove(old_pin_override)

            new_attrib = OrderedDict()
            new_attrib["port_name"] = port
            new_attrib["fc_type"] = fc_type
            new_attrib["fc_val"] = fc_val

            new_pin_override = ET.SubElement(fc_tag, "fc_override", attrib=new_attrib)
            changed = True

        for old_seg_override in old_fc_seg_overrides:

            seg_name = old_seg_override.attrib['name']
            in_val = old_seg_override.attrib['in_val']
            out_val = old_seg_override.attrib['out_val']

            fc_tag.remove(old_seg_override)

            fc_in_type = fc_tag.attrib['default_in_type']
            fc_out_type = fc_tag.attrib['default_out_type']

            pb_type = fc_tag.find("..")
            assert pb_type.tag == "pb_type"

            inputs = pb_type.findall("./input")
            outputs = pb_type.findall("./output")
            clocks = pb_type.findall("./clock")

            for input in inputs + clocks:

                new_attrib = OrderedDict()
                new_attrib["port_name"] = input.attrib['name']
                new_attrib["seg_name"] = seg_name
                new_attrib["fc_type"] = fc_in_type
                new_attrib["fc_val"] = in_val

                fc_override = ET.SubElement(fc_tag, "fc_override", attrib=new_attrib)

            for output in outputs:

                new_attrib = OrderedDict()
                new_attrib["port_name"] = output.attrib['name']
                new_attrib["seg_name"] = seg_name
                new_attrib["fc_type"] = fc_out_type
                new_attrib["fc_val"] = out_val

                fc_override = ET.SubElement(fc_tag, "fc_override", attrib=new_attrib)
        
            changed = True
    return changed

        
def get_port_names(string):
    ports = []

    for elem in string.split():
        #Split off the prefix
        port = elem.split('.')[-1]
        if '[' in port:
            port = port[:port.find('[')]

        ports.append(port)

    return ports



if __name__ == "__main__":
    main()
