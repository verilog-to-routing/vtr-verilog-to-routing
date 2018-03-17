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

INDENT = "    "

#To add support for new upgrades/features add the identifier string here
#and added a guarded call in main()
supported_upgrades = [
    "add_model_timing",
    "upgrade_fc_overrides",
    "upgrade_device_layout",
    "remove_io_chan_distr",
    "upgrade_pinlocations",
    "uniqify_interconnect_names",
    "upgrade_connection_block_input_switch",
    "upgrade_switch_types",
    "rename_fc_attributes",
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

    if arch.tag != "architecture":
        print "Not an architecture file, exiting..."
        sys.exit(1)

    modified = False
    if "add_model_timing" in args.features:
        result = add_model_timing(arch)
        if result:
            modified = True

    if "upgrade_fc_overrides" in args.features:
        result = upgrade_fc_overrides(arch)
        if result:
            modified = True

    if "upgrade_device_layout" in args.features:
        result = upgrade_device_layout(arch)
        if result:
            modified = True

    if "remove_io_chan_distr" in args.features:
        result = remove_io_chan_distr(arch)
        if result:
            modified = True

    if "upgrade_pinlocations" in args.features:
        result = upgrade_pinlocations(arch)
        if result:
            modified = True

    if "uniqify_interconnect_names" in args.features:
        result = uniqify_interconnect_names(arch)
        if result:
            modified = True

    if "upgrade_connection_block_input_switch" in args.features:
        result = upgrade_connection_block_input_switch(arch)
        if result:
            modified = True

    if "upgrade_switch_types" in args.features:
        result = upgrade_switch_types(arch)
        if result:
            modified = True

    if "rename_fc_attributes" in args.features:
        result = rename_fc_attributes(arch)
        if result:
            modified = True

    if modified:
        if args.debug:
            root.write(sys.stdout, pretty_print=args.pretty)
        else:
            with open(args.xml_file, "w") as f:
                root.write(f, pretty_print=args.pretty)

def add_model_timing(arch):
    """
    Records the timing edges specified via timing annotationson primitive PB types,
    and adds the appropriate timing edges to the primitive descriptions in the models
    section.
    """
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
        for xpath_pattern in ["./delay_constant", "./delay_matrix"]:
            for delay_const in prim_pb.findall(xpath_pattern):
                iports = get_port_names(delay_const.attrib['in_port'])
                oports = get_port_names(delay_const.attrib['out_port'])

                for iport in iports:
                    if iport not in primitive_timing_specs[blif_model].comb_edges:
                        primitive_timing_specs[blif_model].comb_edges[iport] = set()

                    for oport in oports:
                        primitive_timing_specs[blif_model].comb_edges[iport].add(oport)

        #Find sequential ports
        for xpath_pattern in ["./T_setup", "./T_clock_to_Q", "./T_hold"]:
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
    """
    Convets the legacy block <fc> pin and segment override specifications,
    to the new unified format.
    """
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
                new_attrib["segment_name"] = seg_name
                new_attrib["fc_type"] = fc_in_type
                new_attrib["fc_val"] = in_val

                fc_override = ET.SubElement(fc_tag, "fc_override", attrib=new_attrib)

            for output in outputs:

                new_attrib = OrderedDict()
                new_attrib["port_name"] = output.attrib['name']
                new_attrib["segment_name"] = seg_name
                new_attrib["fc_type"] = fc_out_type
                new_attrib["fc_val"] = out_val

                fc_override = ET.SubElement(fc_tag, "fc_override", attrib=new_attrib)
        
            changed = True
    return changed

def upgrade_device_layout(arch):
    """
    Upgrades the legacy <gridlocation> specifications (on each pb_type) to the new format
    placed under the <layout> tag.
    """
    changed = False

    #Get the layout tag
    layout = arch.find("./layout")

    #Find all the top level pb_types
    top_pb_types = arch.findall("./complexblocklist/pb_type")

    type_to_grid_specs = OrderedDict()
    for top_pb_type in top_pb_types:
        type_name = top_pb_type.attrib['name']

        assert type_name not in type_to_grid_specs
        type_to_grid_specs[type_name] = []

        gridlocations = top_pb_type.find("gridlocations")
        if gridlocations == None:
            continue

        for child in gridlocations:
            if child.tag is ET.Comment:
                continue
            assert child.tag == "loc"
            type_to_grid_specs[type_name].append(child)

        #Remove the legacy gridlocations from the <pb_type>
        top_pb_type.remove(gridlocations)
        changed = True

    if not changed:
        #No legacy specs to upgrade
        return changed

    device_auto = None
    if 'auto' in layout.attrib:
        aspect_ratio = layout.attrib['auto']

        del layout.attrib['auto']
        change = True

        device_auto = ET.SubElement(layout, 'auto_layout')
        device_auto.attrib['aspect_ratio'] = str(aspect_ratio)
    elif 'width' in layout.attrib and 'height' in layout.attrib:
        width = layout.attrib['width']
        height = layout.attrib['height']

        del layout.attrib['width']
        del layout.attrib['height']
        changed = True

        device_auto = ET.SubElement(layout, 'fixed_layout')
        device_auto.attrib['name'] = "unnamed_device"
        device_auto.attrib['width'] = width
        device_auto.attrib['height'] = height
    else:
        assert False, "Unrecognized <layout> specification"
    
    if 0:
        for type, locs in type_to_grid_specs.iteritems():
            print "Type:", type
            for loc in locs:
                print "\t", loc.tag, loc.attrib

    have_perimeter = False

    for type_name, locs in type_to_grid_specs.iteritems():
        for loc in locs:
            if loc.attrib['type'] == "perimeter":
                have_perimeter = True


    if changed:
        layout.text = "\n" + INDENT
        device_auto.text = "\n" + 2*INDENT
        device_auto.tail = "\n"

    
    for type_name, locs in type_to_grid_specs.iteritems():
        for loc in locs:
            assert loc.tag == "loc"

            loc_type = loc.attrib['type']

            #Note that we scale the priority by a factor of 10 to allow us to 'tweak'
            #the priorities without causing conflicts with user defined priorities
            priority = 10*int(loc.attrib['priority'])

            if loc_type == "col":
                start = loc.attrib['start']

                repeat = None
                if 'repeat' in loc.attrib:
                    repeat = loc.attrib['repeat']

                comment_str = "Column of '{}' with 'EMPTY' blocks wherever a '{}' does not fit.".format(type_name, type_name)

                if have_perimeter:
                    comment_str += " Vertical offset by 1 for perimeter."

                comment = ET.Comment(comment_str)
                device_auto.append(comment)
                comment.tail = "\n" + 2*INDENT

                col_spec = ET.SubElement(device_auto, 'col')

                col_spec.attrib['type'] = type_name
                col_spec.attrib['startx'] = start
                if have_perimeter:
                    col_spec.attrib['starty'] = "1"
                if repeat:
                    col_spec.attrib['repeatx'] = repeat
                col_spec.attrib['priority'] = str(priority)
                col_spec.tail = "\n" + 2*INDENT

                #Classic VPR fills blank spaces (e.g. where a height > 1 block won't fit) with "EMPTY" 
                #instead of with the underlying type. To replicate that we create a col spec with the same 
                #location information, but of type 'EMPTY' and with slightly lower priority than the real type.

                col_empty_spec = ET.SubElement(device_auto, 'col')
                col_empty_spec.attrib['type'] = "EMPTY"
                col_empty_spec.attrib['startx'] = start
                if repeat:
                    col_empty_spec.attrib['repeatx'] = repeat
                if have_perimeter:
                    col_empty_spec.attrib['starty'] = "1"
                col_empty_spec.attrib['priority'] = str(priority - 1) #-1 so it won't override the 'real' col
                col_empty_spec.tail = "\n" + 2*INDENT

            elif loc_type == "rel":
                pos = loc.attrib['pos']

                div_factor = 1. / float(pos)

                int_div_factor = int(div_factor)

                startx = "(W - 1) / {}".format(div_factor)

                if float(int_div_factor) != div_factor:
                    print "Warning: Relative position factor conversion is not exact. Original pos factor: {}. New startx expression: {}".format(pos, startx)

                comment_str = "Column of '{}' with 'EMPTY' blocks wherever a '{}' does not fit.".format(type_name, type_name)

                if have_perimeter:
                    comment_str += " Vertical offset by 1 for perimeter."

                comment = ET.Comment(comment_str)
                device_auto.append(comment)
                comment.tail = "\n" + 2*INDENT

                col_spec = ET.SubElement(device_auto, 'col')
                col_spec.attrib['type'] = type_name
                col_spec.attrib['startx'] = startx
                if have_perimeter:
                    col_spec.attrib['starty'] = "1"
                col_spec.attrib['priority'] = str(priority)
                col_spec.tail = "\n" + 2*INDENT

                #Classic VPR fills blank spaces (e.g. where a height > 1 block won't fit) with "EMPTY" 
                #instead of with the underlying type. To replicate that we create a col spec with the same 
                #location information, but of type 'EMPTY' and with slightly lower priority than the real type.
                col_empty_spec = ET.SubElement(device_auto, 'col')
                col_empty_spec.attrib['type'] = "EMPTY"
                col_empty_spec.attrib['startx'] = startx
                if have_perimeter:
                    col_empty_spec.attrib['starty'] = "1"
                col_empty_spec.attrib['priority'] = str(priority - 1) #-1 so it won't override the 'real' col
                col_empty_spec.tail = "\n" + 2*INDENT

            elif loc_type == "fill":

                comment = ET.Comment("Fill with '{}'".format(type_name))
                device_auto.append(comment)
                comment.tail = "\n" + 2*INDENT

                fill_spec = ET.SubElement(device_auto, 'fill')
                fill_spec.attrib['type'] = type_name
                fill_spec.attrib['priority'] = str(priority)
                fill_spec.tail = "\n" + 2*INDENT

            elif loc_type == "perimeter":
                #The classic VPR perimeter specification did not include the corners (while the new version does)
                # As a result we specify a full perimeter (including corners), and then apply an EMPTY type override
                # at the corners

                comment = ET.Comment("Perimeter of '{}' blocks with 'EMPTY' blocks at corners".format(type_name))
                device_auto.append(comment)
                comment.tail = "\n" + 2*INDENT

                perim_spec = ET.SubElement(device_auto, 'perimeter')
                perim_spec.attrib['type'] = type_name
                perim_spec.attrib['priority'] = str(priority)
                perim_spec.tail = "\n" + 2*INDENT

                corners_spec = ET.SubElement(device_auto, 'corners')
                corners_spec.attrib['type'] = "EMPTY"
                corners_spec.attrib['priority'] = str(priority + 1) #+1 to ensure overrides
                corners_spec.tail = "\n" + 2*INDENT

            else:
                assert False, "Unrecognzied <loc> type tag {}".format(loc_type)

    return changed
        
def remove_io_chan_distr(arch):
    """
    Removes the legacy '<io>' channel width distribution tags
    """
    device = arch.find("./device")
    chan_width_distr = device.find("./chan_width_distr")

    io = chan_width_distr.find("./io")
    if io != None:
        width = float(io.attrib['width'])

        if width != 1.:
            print "Found non-unity io width {}. This is no longer supported by VPR and must be manually correct. Exiting...".format(width)
            sys.exit(1)
        else:
            assert width == 1.
            chan_width_distr.remove(io)
            return True #Modified

def upgrade_pinlocations(arch):
    """
    Upgrades custom pin locations from the 'offset' to 'yoffset' attribute.
    Since previously only width==1 blocks were supported, we only need to consider
    the yoffset case
    """
    modified = False
    pinlocations_list = arch.findall(".//pinlocations")

    for pinlocations in pinlocations_list:
        pb_type = pinlocations.find("..")

        assert pb_type.tag == "pb_type"

        width = 1
        height = 1
        if 'width' in pb_type.attrib:
            width = int(pb_type.attrib['width'])
        if 'height' in pb_type.attrib:
            height = int(pb_type.attrib['height'])

        if width == 1:

            if pinlocations.attrib['pattern'] == "custom":

                for loc in pinlocations:
                    if loc.tag is ET.Comment:
                        continue

                    assert loc.tag == "loc"

                    if 'offset' in loc.attrib:
                        offset = int(loc.attrib['offset'])

                        assert offset < height

                        #Remove the old attribute
                        del loc.attrib['offset']

                        #Add the new attribute
                        loc.attrib['yoffset'] = str(offset)

                        modified = True

            else:
                assert pinlocations.attrib['pattern'] == "spread"

    return modified

def uniqify_interconnect_names(arch):
    """
    Ensure all interconnect tags have unique names
    """
    modified = False

    for interconnect_tag in arch.findall(".//interconnect"):
        seen_names = set()
        cnt = 0
        for child_tag in interconnect_tag:
            if child_tag.tag is ET.Comment:
                continue

            name = orig_name = child_tag.attrib['name']

            if orig_name in seen_names:
                #Generate a unique name
                while name in seen_names:
                    name = orig_name + "_{}".format(cnt)
                    cnt += 1

                assert name not in seen_names
                child_tag.attrib['name'] = name
                modified = True

            seen_names.add(name)

    return modified

def upgrade_connection_block_input_switch(arch):
    """
    Convert connection block input switch specification to use a switch named
    in <switchlist>
    """
    modified = False

    device_tag = arch.find("./device")
    timing_tag = device_tag.find("./timing")
    sizing_tag = device_tag.find("./sizing")
    switchlist_tag = arch.find("./switchlist")
    connection_block_tag = arch.find("./connection_block")

    assert sizing_tag is not None

    if timing_tag is not None:
        assert sizing_tag is not None
        assert 'ipin_mux_trans_size' in sizing_tag.attrib
        assert 'C_ipin_cblock' in timing_tag.attrib
        assert 'T_ipin_cblock' in timing_tag.attrib
        assert not device_tag.find("./connection_block") is not None
        assert switchlist_tag is not None

        R_minW_nmos = sizing_tag.attrib['R_minW_nmos']
        ipin_switch_mux_trans_size = sizing_tag.attrib['ipin_mux_trans_size']
        ipin_switch_cin = timing_tag.attrib['C_ipin_cblock']
        ipin_switch_tdel = timing_tag.attrib['T_ipin_cblock']

        modified = True

        #Remove the old attributes
        del sizing_tag.attrib['ipin_mux_trans_size']
        device_tag.remove(timing_tag)

        #
        #Create the switch
        #
        
        switch_name = "ipin_cblock"

        #Make sure the switch name doesn't already exist
        for switch_tag in switchlist_tag.findall("./switch"):
            assert switch_tag.attrib['name'] != switch_name

        #Comment the switch
        comment = ET.Comment("switch {} resistance set to yeild for 4x minimum drive strength buffer".format(switch_name))
        comment.tail = "\n" + 2*INDENT
        switchlist_tag.append(comment)
        #Create the switch
        switch_tag = ET.SubElement(switchlist_tag, "switch")
        switch_tag.attrib['type'] = 'mux'
        switch_tag.attrib['name'] = switch_name
        switch_tag.attrib['R'] = str(float(R_minW_nmos) / 4.)
        switch_tag.attrib['Cout'] = "0."
        switch_tag.attrib['Cin'] = ipin_switch_cin
        switch_tag.attrib['Tdel'] = ipin_switch_tdel
        switch_tag.attrib['mux_trans_size'] = ipin_switch_mux_trans_size
        switch_tag.attrib['buf_size'] = "auto"
        switch_tag.tail = "\n" + 2*INDENT

        #Create the connection_block tag
        connection_block_tag = ET.SubElement(device_tag, "connection_block")
        connection_block_tag.attrib['input_switch_name'] = switch_name
        connection_block_tag.tail = "\n" + 2*INDENT

    else:
        assert 'ipin_switch_mux_trans_size' not in sizing_tag.attrib

    return modified

def upgrade_switch_types(arch):
    """
    Rename 'buffered' and 'pass_trans' <switch type=""/> to 'tristate' and 'pass_gate'
    """
    modified = False
    switchlist_tag = arch.find("./switchlist")
    assert switchlist_tag is not None

    for switch_tag in switchlist_tag.findall("./switch"):
        
        switch_type = switch_tag.attrib['type']

        if switch_type in ['buffer', 'pass_trans']:
            if switch_type == 'buffer':
                switch_type = "tristate"
            else:
                assert switch_type == 'pass_trans'
                switch_type = "pass_gate"

            switch_tag.attrib['type'] = switch_type
            modified = True

    return modified

def rename_fc_attributes(arch):
    """
    Converts <fc> attributes of the form default_% to %
    """
    fc_tags = arch.findall(".//fc")

    changed = False
    for fc_tag in fc_tags:
        assert fc_tag.tag == "fc"
        attr_to_replace = ["default_in_type", "default_in_val", "default_out_type", "default_out_val"]
        attr_list = list(fc_tag.attrib.keys())
        for attr in attr_list:
            if attr in attr_to_replace:
                val = fc_tag.attrib[attr]
                del fc_tag.attrib[attr]
                fc_tag.attrib[attr.replace("default_", "")] = val
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
