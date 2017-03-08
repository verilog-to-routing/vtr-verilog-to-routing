#!/usr/bin/env python

import argparse
import sys

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
    parser = argparse.ArgumentParser(description="Update VTR architecture files to include explicit sequential port or combinational timing edge specificationstiming on primitive models based on exiting <pb_type> timing specifications")

    parser.add_argument("xml_file", help="xml_file to update (modified in place)")

    return parser.parse_args()

def main():
    args = parse_args()

    print args.xml_file
    root = ET.parse(args.xml_file)

    root_tags = root.findall(".")
    assert len(root_tags) == 1
    arch = root_tags[0]
    assert arch.tag == "architecture"

    models = root.findall("./models/model")

    #Find all primitive pb types
    prim_pbs = root.findall(".//pb_type[@blif_model]")

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
            
    if changed:
        with open(args.xml_file, "w") as f:
            root.write(f)

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
