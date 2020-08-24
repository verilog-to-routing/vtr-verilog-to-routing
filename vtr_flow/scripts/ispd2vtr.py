#!/usr/bin/env python

import argparse
import sys
import os
import re
from collections import OrderedDict
from lxml import etree as ET


class AuxInfo(object):
    nodes_filepath = None
    nets_filepath = None
    weghts_filepath = None
    placement_filepath = None
    scl_filepath = None
    lib_filepath = None


class Cell(object):
    def __init__(self):
        self.name = None
        self.pins = []


class CellPin(object):
    def __init__(self):
        self.name = None
        self.dir = None
        self.is_clock = False
        self.is_ctrl = False
        self.num_pins = 1


class Net(object):
    # Use fixed attributes to save runtime/memory
    __slots__ = ("name", "pins")

    def __init__(self):
        self.name = None
        self.pins = []


class NetPin(object):
    # Use fixed attributes to save runtime/memory
    __slots__ = ("inst_name", "pin_name")

    def __init__(self):
        self.inst_name = None
        self.pin_name = None


class NodePin(object):
    # Use fixed attributes to save runtime/memory
    __slots__ = ("pin_name", "net_name")

    def __init__(self):
        self.pin_name = None
        self.net_name = None


class DeviceGrid(object):
    def __init__(self):
        self.width = None
        self.height = None
        self.sites = []


class DeviceResource(object):
    def __init__(self):
        self.name = None
        self.primitives = []


class DeviceSite(object):
    def __init__(self):
        self.name = None
        self.resources = []


def is_input(cell_type):
    if cell_type == "IBUF":
        return True
    return False


def is_output(cell_type):
    if cell_type == "OBUF":
        return True
    return False


def parse_args():

    parser = argparse.ArgumentParser(
        "Script to convert ISPD FPGA Bookshelf format benchmarks into VTR compatible formats"
    )

    parser.add_argument("aux_file", help=".aux file describing the benchmark")
    parser.add_argument("--benchmark", default=None, help="File name for output .blif file")
    parser.add_argument("--arch", default=None, help="Filename for a partial VTR architecture")
    parser.add_argument("--constraints", default=None, help="Filename for writing constraints")
    parser.add_argument(
        "--merge_arch_ports",
        default=True,
        help="Merge multi-bit ports in architecture. Default: %(default)s",
    )

    return parser.parse_args()


def main():
    args = parse_args()

    aux_info = parse_aux(args.aux_file)

    if args.benchmark:
        bookshelf2blif(aux_info, args.benchmark, merge_ports=args.merge_arch_ports)

    if args.arch:
        bookshelf2arch(aux_info, args.arch, merge_ports=args.merge_arch_ports)

    if not args.arch and not args.benchmark:
        assert False, "Must specify one of --arch or --benchmark"


def parse_aux(aux_filepath):

    dirname = os.path.dirname(aux_filepath)

    aux_info = AuxInfo()

    with open(aux_filepath) as f:
        for line in f:
            if line.startswith("#"):
                continue
            if line.startswith("design"):
                prefix, info = line.split(":")

                for filename in info.split():
                    filepath = os.path.join(dirname, filename)
                    if filename.endswith(".nodes"):
                        aux_info.nodes_filepath = filepath
                    elif filename.endswith(".nets"):
                        aux_info.nets_filepath = filepath
                    elif filename.endswith(".wts"):
                        aux_info.weights_filepath = filepath
                    elif filename.endswith(".pl"):
                        aux_info.placement_filepath = filepath
                    elif filename.endswith(".scl"):
                        aux_info.scl_filepath = filepath
                    elif filename.endswith(".lib"):
                        aux_info.lib_filepath = filepath
                    else:
                        assert False, "Unrecognized file type"
            else:
                assert False, "Unrecognized line in .aux file"

    return aux_info


def bookshelf2blif(aux_info, out_filepath, merge_ports=False):
    cells = parse_lib(aux_info, merge_ports)

    nodes = parse_nodes(aux_info)
    nets = parse_nets(aux_info)

    # Inferr PIs/POs which are implicit in bookshelf format
    inputs = OrderedDict()
    outputs = OrderedDict()
    for node_name, type in nodes.iteritems():
        net_name = None
        pin_name = None
        if is_input(type):
            net_name = node_name + "_in"
            inputs[node_name] = net_name
            pin_name = "I"
        elif is_output(type):
            net_name = node_name + "_out"
            outputs[node_name] = net_name
            pin_name = "O"
        else:
            continue

        assert net_name is not None
        assert pin_name is not None

        # Create the net
        net = Net()
        net.name = net_name
        net_pin = NetPin()
        net_pin.inst_name = node_name
        net_pin.pin_name = pin_name

        net.pins.append(net_pin)

        nets.append(net)

    # Node to net mapping
    node_pins = OrderedDict()
    for net in nets:
        for net_pin in net.pins:
            assert net_pin.inst_name is not None
            assert net_pin.pin_name is not None
            if net_pin.inst_name not in node_pins:
                node_pins[net_pin.inst_name] = []

            node_pin = NodePin()
            node_pin.net_name = net.name
            node_pin.pin_name = net_pin.pin_name
            node_pins[net_pin.inst_name].append(node_pin)

    with open(out_filepath, "w") as f:
        print >> f, "#Generated with {}".format(" ".join(sys.argv))
        print >> f, ""
        print >> f, ".model top"

        print >> f, ".inputs \\"
        pretty_print_list(f, inputs.values())
        print >> f, ".outputs \\"
        pretty_print_list(f, outputs.values())

        for node_name, type in nodes.iteritems():
            print >> f, ""
            print >> f, "#{}".format(node_name)
            print >> f, ".subckt {} \\".format(type)

            pins = sorted(node_pins[node_name], key=lambda x: x.pin_name)
            for i, node_pin in enumerate(pins):

                print >> f, "    {}={}".format(node_pin.pin_name, node_pin.net_name),

                if i != len(node_pins[node_name]) - 1:
                    print >> f, "\\"
                else:
                    print >> f, ""

        print >> f, ".end "
        print >> f, ""

        for cell in cells:
            print >> f, ""
            print >> f, ".model {}".format(cell.name)

            input_names = [cell_pin.name for cell_pin in cell.pins if cell_pin.dir == "INPUT"]
            output_names = [cell_pin.name for cell_pin in cell.pins if cell_pin.dir == "OUTPUT"]
            print >> f, ".inputs \\"
            pretty_print_list(f, input_names)
            print >> f, ".outputs \\"
            pretty_print_list(f, output_names)
            print >> f, ".blackbox"
            print >> f, ".end"
        print >> f, "#EOF"


def bookshelf2arch(aux_info, out_filepath, merge_ports=False):
    cells = parse_lib(aux_info, merge_ports=merge_ports)

    grid, sites, resources = parse_scl(aux_info)

    root = ET.Element("architecture")
    for cell in cells:
        add_arch_model(root, cell)

    for site in sites:
        add_arch_block(root, site, resources, cells)

    add_arch_grid(root, grid)

    with open(out_filepath, "w") as f:
        f.write(ET.tostring(root, pretty_print=True))


def parse_lib(aux_info, merge_ports=False):
    index_regex = re.compile(r"(?P<name>.*)\[(?P<index>\d+)\]")
    cells = []
    with open(aux_info.lib_filepath) as f:
        cell = None
        for line in f:
            if line.startswith("CELL"):
                cell_name = line.split(" ")[1]
                cell = Cell()
                cell.name = cell_name.strip("\n")
            elif line.startswith("  PIN"):

                pin = CellPin()
                tokens = line.split()
                assert len(tokens) >= 3
                for i, token in enumerate(tokens):
                    token = token.strip("\n")

                    if i == 0:
                        assert token == "PIN"
                    elif i == 1:
                        pin.name = token
                    elif i == 2:
                        pin.dir = token
                    elif i == 3:
                        if token == "CLOCK":
                            pin.is_clock = True
                        else:
                            pin.is_clock = False

                        if token == "CTRL":
                            pin.is_ctrl = True
                        else:
                            pin.is_ctrl = False
                    else:
                        assert False, "Unrecognzied pin attribute"

                cell.pins.append(pin)
            elif line.startswith("END CELL"):
                cells.append(cell)
                cell = None

        if merge_ports:
            for i in xrange(len(cells)):
                ports = OrderedDict()

                for pin in cells[i].pins:
                    match = index_regex.match(pin.name)
                    if match:
                        port_name = match.groupdict()["name"]
                        index = match.groupdict()["index"]
                        if port_name not in ports:
                            ports[port_name] = []
                        ports[port_name].append(pin)
                    else:
                        if pin.name not in ports:
                            ports[pin.name] = []
                        ports[pin.name].append(pin)

                cells[i].pins = []
                for port_name, pins in ports.iteritems():
                    # All pins in port must have same: dir/is_clock/is_ctrl
                    dir = None
                    is_clock = None
                    is_ctrl = None
                    for pin in pins:
                        if not dir:
                            dir = pin.dir
                            is_clock = pin.is_clock
                            is_ctrl = pin.is_ctrl
                        else:
                            assert dir == pin.dir
                            assert is_clock == pin.is_clock
                            assert is_ctrl == pin.is_ctrl

                    port = CellPin()
                    port.name = port_name
                    port.dir = dir
                    port.is_clock = is_clock
                    port.is_ctrl = is_ctrl
                    port.num_pins = len(pins)

                    cells[i].pins.append(port)
                    # print '#', cells[i].name, port.name, port.num_pins

                # for pin in cells[i].pins:
                # print '*', cells[i].name, pin.name, pin.num_pins

    # for cell in cells:
    # for pin in cell.pins:
    # print '!', cell.name, pin.name, pin.num_pins
    return cells


def parse_nodes(aux_info):
    nodes = OrderedDict()
    with open(aux_info.nodes_filepath) as f:
        for line in f:
            inst_name, type = line.split()

            assert inst_name is not None
            assert type is not None

            nodes[inst_name] = type

    return nodes


def parse_nets(aux_info):
    nets = []

    with open(aux_info.nets_filepath) as f:
        net = None
        for line in f:
            if line.startswith("#"):
                continue
            elif line.startswith("net"):
                net_token, net_name, num_pins = line.split()
                assert net_name is not None

                net = Net()
                net.name = net_name
                net.pins = []
            elif line.startswith("endnet"):
                nets.append(net)
                net = None
            else:
                # Assume net pin
                inst_name, pin_name = line.split()
                assert pin_name is not None
                net_pin = NetPin()
                net_pin.inst_name = inst_name
                net_pin.pin_name = pin_name
                net.pins.append(net_pin)
    return nets


def parse_scl(aux_info):
    grid = None
    sites = []
    resources = []

    with open(aux_info.scl_filepath) as f:
        for line in f:

            # SITEMAP
            if line.startswith("SITEMAP"):
                sitemap_token, width, height = line.split()

                assert not grid
                grid = DeviceGrid()
                grid.width = width
                grid.height = height

                for line in f:
                    if line.startswith("END SITEMAP"):
                        break

                    x, y, site = line.split()

                    grid.sites.append((x, y, site))

            # SITE parsing
            elif line.startswith("SITE"):
                site_token, site_type = line.split()

                site = DeviceSite()
                site.name = site_type

                for line in f:
                    if line.startswith("END SITE"):
                        break
                    resource, count = line.split()
                    site.resources.append((resource, count))
                sites.append(site)

            # RESOURCE parsing
            elif line.startswith("RESOURCES"):
                for line in f:
                    if line.startswith("END RESOURCES"):
                        break

                    resource_info = line.split()

                    resource = DeviceResource()
                    resource.name = resource_info[0]
                    resource.primitives = resource_info[1:]

                    resources.append(resource)

    return grid, sites, resources


def add_arch_model(root, cell):
    assert root.tag == "architecture"

    # Create models if first model
    models = root.find("models")
    if models is None:
        models = ET.SubElement(root, "models")

    # Initialize the current model
    model = ET.SubElement(models, "model")
    model.set("name", cell.name)

    input_pins = [pin for pin in cell.pins if pin.dir == "INPUT"]
    output_pins = [pin for pin in cell.pins if pin.dir == "OUTPUT"]

    # Find the clock, assume all ports sequential
    clock_name = None
    for ipin in input_pins:
        if ipin.is_clock:
            assert clock_name == None, "Primitive with multiple clocks"
            clock_name = ipin.name

    # Create each input pin
    input_ports = ET.SubElement(model, "input_ports")
    for ipin in input_pins:
        port = ET.SubElement(input_ports, "port")
        port.set("name", ipin.name)
        if ipin.is_clock:
            port.set("is_clock", "1")
        elif clock_name != None:
            port.set("clock", clock_name)

    # Create each output pin
    output_ports = ET.SubElement(model, "output_ports")
    for opin in output_pins:
        assert not opin.is_clock
        assert not opin.is_ctrl

        port = ET.SubElement(output_ports, "port")
        port.set("name", opin.name)

        if clock_name != None:
            port.set("clock", clock_name)


def add_arch_block(root, site, resources, cells):
    # Create complexblocklist if first block
    cblist = root.find("complexblocklist")
    if cblist is None:
        cblist = ET.SubElement(root, "complexblocklist")

    # Create the top-level PB type
    pb_type = ET.SubElement(cblist, "pb_type")
    pb_type.set("name", site.name)

    for resource, count in site.resources:
        matching_resources = [r for r in resources if r.name == resource]
        assert len(matching_resources) == 1
        child_pb_type = create_resource(matching_resources[0], cells)
        child_pb_type.set("num_pb", count)

        pb_type.append(child_pb_type)


def create_resource(resource, cells):

    pb_type = ET.Element("pb_type")
    pb_type.set("name", resource.name)

    for prim in resource.primitives:
        mode = ET.SubElement(pb_type, "mode")
        mode.set("name", prim)

        prim_pb_type = ET.SubElement(mode, "pb_type")
        prim_pb_type.set("name", prim)
        prim_pb_type.set("blif_model", ".subckt {}".format(prim))

        matching_cells = [cell for cell in cells if cell.name == prim]
        assert len(matching_cells) == 1
        cell = matching_cells[0]

        inputs = [pin for pin in cell.pins if pin.dir == "INPUT"]
        for ipin in inputs:
            input = ET.SubElement(prim_pb_type, "input")
            input.set("name", ipin.name)
            input.set("num_pins", str(ipin.num_pins))

        outputs = [pin for pin in cell.pins if pin.dir == "OUTPUT"]
        for opin in outputs:
            output = ET.SubElement(prim_pb_type, "output")
            output.set("name", opin.name)
            output.set("num_pins", str(opin.num_pins))
    return pb_type


def add_arch_grid(root, grid):
    layout = root.find("layout")
    assert not layout

    layout = ET.SubElement(root, "layout")
    fixed_layout = ET.SubElement(layout, "fixed_layout")
    fixed_layout.set("width", grid.width)
    fixed_layout.set("height", grid.height)
    fixed_layout.set("name", "ultrascale")

    for x, y, site in grid.sites:
        single = ET.SubElement(fixed_layout, "single")
        single.set("type", site)
        single.set("priority", "1")
        single.set("x", x)
        single.set("y", y)


def pretty_print_list(f, list, indent="    "):
    for i, item in enumerate(list):
        print >> f, "{}{}".format(indent, item),
        if i == len(list) - 1:
            print >> f, ""
        else:
            print >> f, "\\"


if __name__ == "__main__":
    main()
