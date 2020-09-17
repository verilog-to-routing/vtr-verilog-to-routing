"""
This file defines various GDB printers for some useful VTR/VPR
data structures. It makes them print nicely when you try to display
such data structures when debugging within GDB.

For more details on how to use this see:

    https://docs.verilogtorouting.org/en/latest/dev/developing#vtr-pretty-printers
"""
import re

# VTR related
class VtrStrongIdPrinter:
    def __init__(self, val, typename="vtr::StrongId"):
        self.val = val
        self.typename = typename

    def to_string(self):
        id = self.val["id_"]

        invalid_id = self.val.type.template_argument(2)

        if id == invalid_id:
            return self.typename + "(INVALID)"
        else:
            return self.typename + "(" + str(id) + ")"


class TatumStrongIdPrinter:
    def __init__(self, val, typename="tatum::util::StrongId"):
        self.val = val
        self.typename = typename

    def to_string(self):
        id = self.val["id_"]

        invalid_id = self.val.type.template_argument(2)

        if id == invalid_id:
            return self.typename + "(INVALID)"
        else:
            return self.typename + "(" + str(id) + ")"


# class VtrNdMatrixPrinter:
# def __init__(self, val):
# self.val = val

# def to_string(self):
# id = self.val['id_']

# ndim = self.val.type.template_argument(2)

# if(id == invalid_id):
# return self.typename + "(INVALID)"
# else:
# return self.typename + "(" + str(id) + ")"


def vtr_type_lookup(val):
    # unqualified() removes qualifiers like 'const'
    # strip_typedefs() returns the real type
    type = str(val.type.unqualified().strip_typedefs())

    if type.startswith("vtr::StrongId"):
        tag = val.type.template_argument(0)
        tag_type = str(tag)

        if tag_type.startswith("atom_block"):
            return VtrStrongIdPrinter(val, "AtomBlockId")
        elif tag_type.startswith("atom_port"):
            return VtrStrongIdPrinter(val, "AtomPortId")
        elif tag_type.startswith("atom_pin"):
            return VtrStrongIdPrinter(val, "AtomPinId")
        elif tag_type.startswith("atom_net"):
            return VtrStrongIdPrinter(val, "AtomNetId")
        elif tag_type.startswith("cluster_block"):
            return VtrStrongIdPrinter(val, "ClusterBlockId")
        elif tag_type.startswith("cluster_port"):
            return VtrStrongIdPrinter(val, "ClusterPortId")
        elif tag_type.startswith("cluster_pin"):
            return VtrStrongIdPrinter(val, "ClusterPinId")
        elif tag_type.startswith("cluster_net"):
            return VtrStrongIdPrinter(val, "ClusterNetId")
        elif tag_type.startswith("rr_node"):
            return VtrStrongIdPrinter(val, "RRNodeId")
        elif tag_type.startswith("rr_edge"):
            return VtrStrongIdPrinter(val, "RREdgeId")
        elif tag_type.startswith("rr_switch"):
            return VtrStrongIdPrinter(val, "RRSwitchId")
        elif tag_type.startswith("rr_segment"):
            return VtrStrongIdPrinter(val, "RRSegmentId")
        elif tag_type.startswith("cluster_net"):
            return VtrStrongIdPrinter(val, "ClusterNetId")
        elif tag_type.startswith("pack_molecule_block"):
            return VtrStrongIdPrinter(val, "MoleculeBlockId")
        elif tag_type.startswith("pack_molecule_edge"):
            return VtrStrongIdPrinter(val, "MoleculeEdgeId")
        elif tag_type.startswith("pack_molecule_pin"):
            return VtrStrongIdPrinter(val, "MoleculePinId")
        elif tag_type.startswith("pack_molecule_id"):
            return VtrStrongIdPrinter(val, "PackMoleculeId")
        else:
            return VtrStrongIdPrinter(val)

    elif type.startswith("tatum::util::StrongId"):
        tag = val.type.template_argument(0)
        tag_type = str(tag)

        if tag_type.startswith("tatum::node_id"):
            return TatumStrongIdPrinter(val, "tatum::NodeId")
        elif tag_type.startswith("tatum::edge_id"):
            return TatumStrongIdPrinter(val, "tatum::EdgeId")
        elif tag_type.startswith("tatum::domain_id"):
            return TatumStrongIdPrinter(val, "tatum::DomainId")
        elif tag_type.startswith("tatum::level_id"):
            return TatumStrongIdPrinter(val, "tatum::LevelId")
        else:
            return TatumStrongIdPrinter(val)
    # elif type.startswith("vtr::NdMatrix"):
    # return VtrNdMatrixPrinter(val)
    else:
        # print("Type '{}'".format(type))
        pass

    return None
