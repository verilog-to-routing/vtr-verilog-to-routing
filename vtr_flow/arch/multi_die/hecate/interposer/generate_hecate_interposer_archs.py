#!/usr/bin/python3

"""Generate Hecate interposer architecture variants from a CSV description
and the base 'hecate_25d_L17_int_10um_bump_fanin_12.xml' architecture."""

import csv
import re
import os
from typing import Any, Mapping, Sequence, Tuple
from lxml import etree

# pylint: disable=c-extension-no-member


def configure_interposer_segment(xml_root: Any, segment_length: int, mux_name: str) -> None:
    """Configure the physical interposer segment definition.

    Updates the ``int_wire`` segment in the architecture XML so that its
    length, mux name, switch-block pattern, and connection-block pattern match
    the interposer wire described by the current CSV row.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        segment_length: Length to assign to the ``int_wire`` segment.
        mux_name: Name to assign to the segment's nested ``mux`` tag, when one
            exists.

    Returns:
        None. The XML tree is modified in place. If no ``int_wire`` segment is
        found, the function leaves the tree unchanged.
    """
    segment = xml_root.xpath(".//segmentlist/segment[@name='int_wire']")
    if not segment:
        return

    seg = segment[0]
    seg.set("length", str(segment_length))

    mux_tag = seg.find("mux")
    if mux_tag is not None:
        mux_tag.set("name", mux_name)

    # Pattern logic: sb = L+1, cb = L
    sb_tag = seg.xpath("./sb[@type='pattern']")
    if sb_tag:
        sb_tag[0].text = " ".join(["0"] * (segment_length + 1))

    cb_tag = seg.xpath("./cb[@type='pattern']")
    if cb_tag:
        cb_tag[0].text = " ".join(["0"] * segment_length)


def configure_scatter_gather_patterns(
    xml_root: Any, segment_length: int, gather_n_val: str, scatter_n_val: str
) -> None:
    """Configure interposer scatter-gather connectivity patterns.

    Updates the upward and downward interposer scatter-gather patterns so their
    gather/scatter connection counts and scatter-gather link offsets match the
    generated architecture variant.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        segment_length: Interposer wire segment length. This determines the
            signed ``y_offset`` used by each scatter-gather link.
        gather_n_val: Number of gather connections to write to each matching
            ``gather/wireconn`` tag.
        scatter_n_val: Number of scatter connections to write to each matching
            ``scatter/wireconn`` tag.

    Returns:
        None. The XML tree is modified in place. Missing scatter-gather
        patterns or child tags are skipped.
    """
    patterns = {
        "downward": xml_root.xpath(
            ".//scatter_gather_list/sg_pattern[@name='interposer_sg_downward']"
        ),
        "upward": xml_root.xpath(".//scatter_gather_list/sg_pattern[@name='interposer_sg_upward']"),
    }

    for direction, sg_list in patterns.items():
        if not sg_list:
            continue
        sg = sg_list[0]

        # Update num_conns for gather and scatter (3, 6, 12, 24, or 38)
        for tag_name in ["gather", "scatter"]:
            wireconn = sg.xpath(f"./{tag_name}/wireconn")
            if wireconn:
                if tag_name == "gather":
                    wireconn[0].set("num_conns", gather_n_val)
                else:
                    wireconn[0].set("num_conns", scatter_n_val)

        # Update the sg_link Y offsets depending on the interposer routing direction
        sg_link = sg.xpath("./sg_link_list/sg_link")
        if sg_link:
            offset_val = -segment_length if direction == "downward" else segment_length
            sg_link[0].set("y_offset", str(offset_val))


def configure_interdie_wires(
    xml_root: Any, layout_xpath: str, segment_length: int, csv_num: str
) -> None:
    """Configure interposer cut definition for a layout.

    Updates the upward and downward ``interdie_wire`` tags under the selected
    layout's ``interposer_cut``. The layout is selected by XPath so the same
    logic can be reused for any layout tag selected by XPath.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        layout_xpath: XPath selecting the layout element to update, such as
            ``.//layout/auto_layout/``. A trailing slash is optional.
        segment_length: Interposer wire segment length. This determines the
            offset values of the interdie_wire tag.
        csv_num: Number of inter-die wires instantiated at each switchblock location.
            Determines the 'num' value of the interdie_wire tag.

    Returns:
        None. The XML tree is modified in place. Missing upward or downward
        ``interdie_wire`` tags are skipped.
    """
    layout_xpath = layout_xpath.rstrip("/")
    up_wire = xml_root.xpath(
        f"{layout_xpath}/interposer_cut/interdie_wire[@sg_name='interposer_sg_upward']"
    )
    if up_wire:
        up_wire[0].set("offset_start", str(-(segment_length - 1)))
        up_wire[0].set("offset_end", "-1")
        up_wire[0].set("num", csv_num)

    down_wire = xml_root.xpath(
        f"{layout_xpath}/interposer_cut/interdie_wire[@sg_name='interposer_sg_downward']"
    )
    if down_wire:
        down_wire[0].set("offset_start", "1")
        down_wire[0].set("offset_end", str(segment_length - 1))
        down_wire[0].set("num", csv_num)


def write_arch(xml_tree: Any, output_dir: str, arch_id: str, gather_n_val: str) -> None:
    """Write a generated architecture XML file to disk.

    Builds the output filename from the architecture identifier and gather
    fan-in value, then writes the XML tree with an XML declaration and pretty
    formatting.

    Args:
        xml_tree: Parsed architecture XML tree to serialize.
        output_dir: Directory where the generated architecture should be
            written.
        arch_id: Architecture identifier from the CSV row. This becomes part of
            the generated filename.
        gather_n_val: Gather fan-in value. This becomes the ``fanin`` suffix in
            the generated filename.

    Returns:
        None. The XML file is written to ``output_dir`` and the generated path
        is printed.
    """
    output_filename = f"hecate_25d_{arch_id}_fanin_{gather_n_val}.xml"
    output_path = os.path.join(output_dir, output_filename)

    xml_tree.write(output_path, encoding="UTF-8", xml_declaration=True, pretty_print=True)
    print(f"Generated: {output_path}")


def generate_archs_for_row(
    row: Mapping[str, str],
    template_path: str,
    output_dir: str,
    parser: Any,
    connection_sizes: Sequence[Tuple[str, str]],
) -> None:
    """Generate all architecture variants described by one CSV row.

    Extracts the interposer segment length and connectivity settings from a
    CSV row, then generates one architecture file for each gather/scatter
    connection-size pair. Each output starts from a fresh parse of the template
    XML so variants do not inherit modifications from earlier iterations.

    Args:
        row: CSV row containing at least ``arch_id``, ``mux_name``,
            ``wire_name``, and ``num`` fields.
        template_path: Path to the base Hecate architecture XML template.
        output_dir: Directory where generated architecture XML files should be
            written.
        parser: ``lxml.etree`` parser used to read the template while
            preserving comments.
        connection_sizes: Sequence of ``(gather_n_val, scatter_n_val)`` pairs
            to generate for this row.

    Returns:
        None. Generated architecture files are written to ``output_dir``.
    """
    arch_id = row["arch_id"]
    mux_name = row["mux_name"]
    wire_name = row["wire_name"]
    csv_num = row["num"]

    # Extract the interposer segment length
    match = re.search(r"\d+", wire_name)
    segment_length = int(match.group()) if match else 1

    for gather_n_val, scatter_n_val in connection_sizes:
        # Initialize a fresh XML tree from the template for each configuration
        xml_tree = etree.parse(template_path, parser)
        xml_root = xml_tree.getroot()

        configure_interposer_segment(xml_root, segment_length, mux_name)
        configure_scatter_gather_patterns(xml_root, segment_length, gather_n_val, scatter_n_val)
        configure_interdie_wires(xml_root, ".//layout/auto_layout/", segment_length, csv_num)
        write_arch(xml_tree, output_dir, arch_id, gather_n_val)


def generate_hecate_interposer_archs(
    csv_file: str, template_path: str, output_dir: str = "hecate_25D"
) -> None:
    """Generate Hecate interposer architecture XML variants.

    Reads a CSV description of interposer wire configurations and applies each
    row to a base Hecate architecture template. For each row, this function
    emits variants for the configured gather/scatter fan-in and fan-out
    exploration points.

    Args:
        csv_file: Path to the CSV file containing architecture variant inputs.
        template_path: Path to the base Hecate architecture XML template.
        output_dir: Directory where generated architecture XML files should be
            written. Created if it does not already exist.

    Returns:
        None. Generated architecture files are written to ``output_dir``. If
        ``template_path`` does not exist, an error is printed and no files are
        generated.
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    if not os.path.exists(template_path):
        print(f"Error: Template {template_path} not found.")
        return

    # Using lxml parser to retain existing XML comments during tree transformation
    parser = etree.XMLParser(remove_comments=False)

    # Define the range of fan-in and fan-out sizes for interposer routing exploration
    gather_conn_sizes = ["3", "6", "12", "24", "48"]
    scatter_conn_sizes = ["4", "8", "16", "32", "64"]
    connection_sizes = list(zip(gather_conn_sizes, scatter_conn_sizes))

    with open(csv_file, mode="r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)

        for row in reader:
            generate_archs_for_row(row, template_path, output_dir, parser, connection_sizes)


if __name__ == "__main__":
    generate_hecate_interposer_archs(
        "int_connectivity.csv", "hecate_25d_L17_int_10um_bump_fanin_12.xml"
    )
