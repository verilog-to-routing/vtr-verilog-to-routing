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

def parse_args():
    parser = argparse.ArgumentParser(description="Update VTR architecture files to use the updated <fc_override> tags")

    parser.add_argument("xml_file", help="xml_file to update (modified in place)")
    parser.add_argument("--debug", default=False, action="store_true", help="Print to stdout instead of modifying file inplace")
    parser.add_argument("--pretty", default=False, action="store_true", help="Pretty print the output?")

    return parser.parse_args()

def main():
    args = parse_args()

    print args.xml_file
    parser = ET.XMLParser()
    root = ET.parse(args.xml_file, parser)


    changed = False

    root_tags = root.findall(".")
    assert len(root_tags) == 1
    arch = root_tags[0]
    assert arch.tag == "architecture"

    fc_tags = root.findall(".//fc")

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
        
    if changed:
        if args.debug:
            root.write(sys.stdout, pretty_print=args.pretty)
        else:
            with open(args.xml_file, "w") as f:
                root.write(f, pretty_print=args.pretty)

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
