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
    """Configure the physical interposer segment properties."""
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
    """Configure scatter-gather fan-in, fan-out, and offsets."""
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
    """Configure legal interposer cut wire ranges."""
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
    """Write a generated architecture XML file."""
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
    """Generate all Hecate interposer architecture variants for one CSV row."""
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
        configure_interdie_wires(
            xml_root, ".//layout/fixed_layout[@name='hecate_small']/", segment_length, csv_num
        )
        write_arch(xml_tree, output_dir, arch_id, gather_n_val)


def generate_hecate_interposer_archs(
    csv_file: str, template_path: str, output_dir: str = "hecate_25D"
) -> None:
    """
    Generates customized VPR architecture XML files by injecting varying scatter-gather
    and inter-die connectivity configurations into a base template XML.
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
