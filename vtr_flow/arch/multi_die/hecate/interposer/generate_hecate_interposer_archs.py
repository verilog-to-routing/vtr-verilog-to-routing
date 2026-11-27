#!/usr/bin/python3

"""Generate Hecate interposer architecture variants from a CSV description
and the base Hecate architectures:
  - 'hecate_25d_L18_int_10um_bump_unidir_fanin_32.xml' (2 dies, 1 interposer cut)
  - 'hecate_25d_L18_int_10um_bump_unidir_1x4_fanin_32.xml' (1x4 dies, 3 interposer cuts)
  - 'hecate_25d_L18_int_10um_bump_unidir_2x2_fanin_32.xml' (2x2 dies, 1 horizontal + 1 vertical cut)"""

import csv
import re
import os
from typing import Any, Mapping, Sequence, Tuple
from lxml import etree

# pylint: disable=c-extension-no-member

# Scatter-gather pattern name -> (sg_link offset attribute, offset sign).
# Up/down patterns cross horizontal cuts; left/right patterns cross vertical cuts.
SG_PATTERN_DIRECTIONS = {
    "interposer_sg_downward": ("y_offset", -1),
    "interposer_sg_upward": ("y_offset", 1),
    "interposer_sg_leftward": ("x_offset", -1),
    "interposer_sg_rightward": ("x_offset", 1),
}


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
    xml_root: Any, segment_length: int, gather_n_val: str, scatter_n_val: str, directionality: str
) -> None:
    """Configure interposer scatter-gather connectivity patterns.

    Updates the up/down/left/right interposer scatter-gather patterns (whichever
    exist in the template) so their
    gather/scatter connection counts and scatter-gather link offsets match the
    generated architecture variant.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        segment_length: Interposer wire segment length. This determines the
            signed ``y_offset`` (up/down) or ``x_offset`` (left/right) used by
            each scatter-gather link.
        gather_n_val: Number of gather connections to write to each matching
            ``gather/wireconn`` tag.
        scatter_n_val: Number of scatter connections to write to each matching
            ``scatter/wireconn`` tag.

    Returns:
        None. The XML tree is modified in place. Missing scatter-gather
        patterns or child tags are skipped.
    """
    for pattern_name, (offset_attr, sign) in SG_PATTERN_DIRECTIONS.items():
        sg_list = xml_root.xpath(
            f".//scatter_gather_list/sg_pattern[@name='{pattern_name}']"
        )
        if not sg_list:
            continue
        sg = sg_list[0]

        # set directionality
        if (directionality == "unidir"):
            sg.set("type", "unidir")
        else:
            sg.set("type", "bidir")

        # Update num_conns for gather and scatter (3, 6, 12, 24, or 38)
        for tag_name in ["gather", "scatter"]:
            wireconn = sg.xpath(f"./{tag_name}/wireconn")
            if wireconn:
                if tag_name == "gather":
                    wireconn[0].set("num_conns", gather_n_val)
                else:
                    wireconn[0].set("num_conns", scatter_n_val)
        # Update the sg_link X/Y offset depending on the interposer routing direction
        sg_link = sg.xpath("./sg_link_list/sg_link")
        if sg_link:
            sg_link[0].set(offset_attr, str(sign * segment_length))


def configure_interdie_wires(
    xml_root: Any, layout_xpath: str, segment_length: int, csv_num: str
) -> None:
    """Configure interposer cut definitions for a layout.

    Updates the up/down/left/right ``interdie_wire`` tags under every
    ``interposer_cut`` of the selected layout (e.g. one cut for a 2-die
    system, three cuts for a 1x4 system, one horizontal and one vertical cut
    for a 2x2 system). Wires going up/right start ``segment_length - 1``
    tiles before the cut; wires going down/left start just after it. The layout is selected by XPath so the same
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
        None. The XML tree is modified in place. Missing ``interdie_wire``
        tags are skipped.
    """
    layout_xpath = layout_xpath.rstrip("/")
    for pattern_name, (_, sign) in SG_PATTERN_DIRECTIONS.items():
        if sign > 0:
            offset_start, offset_end = -(segment_length - 1), -1
        else:
            offset_start, offset_end = 1, segment_length - 1
        for wire in xml_root.xpath(
            f"{layout_xpath}/interposer_cut/interdie_wire[@sg_name='{pattern_name}']"
        ):
            wire.set("offset_start", str(offset_start))
            wire.set("offset_end", str(offset_end))
            wire.set("num", csv_num)


def _offset_expr(base: str, delta: int) -> str:
    """Return a grid expression string for ``base + delta`` (e.g. ``"W / 2 - 19"``)."""
    if delta == 0:
        return base
    return f"{base} {'+' if delta > 0 else '-'} {abs(delta)}"


def configure_segmented_interdie_wires(
    xml_root: Any, layout_xpath: str, segment_length: int, csv_num: str
) -> None:
    """Configure interposer cuts whose interdie wiring varies along the cut.

    Used for the 2x2 system, where the horizontal and vertical cuts cross at
    the middle of the device. The goal is that every location within
    ``L - 1`` of any cut (where ``L`` is ``segment_length``) has exactly
    ``2 * num`` scatter-gather endpoints (gathers plus scatters, over all cuts
    and directions). A single cut gives ``num`` gathers of one direction plus
    ``num`` scatters of the other at each such location, so where the two
    cuts' bands overlap each cut must supply only half.

    For each direction under each cut, the single template ``interdie_wire``
    tag is replaced by these tags along the cut (``mid`` is the position of the
    crossing cut, ``W / 2 - 1`` or ``H / 2 - 1``):

      1. ``[0, mid - L]``: ``num`` at every location.
      2. ``[mid - L + 1, mid + L - 1]``, every other location: half of ``num``.
      3. ``[mid - L + 2, mid + L - 1]``, the locations skipped by (2): half of ``num``.
      4. ``mid`` only: tops (2)/(3) back up to ``num``, since the crossing
         cut's band does not include its own cut line.
      5. ``[mid + L, end]``: ``num`` at every location.

    Ranges (2) and (3) together cover exactly the crossing cut's band
    ``[mid - L + 1, mid - 1] U [mid + 1, mid + L - 1]`` (plus ``mid``, fixed
    by (4)). When ``num`` is odd, up/right wires get the extra wire in range (2)
    and down/left wires get it in range (3), so each location still totals
    ``num`` over both directions. Tags whose ``num`` would be 0 are omitted.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        layout_xpath: XPath to the layout tag containing ``interposer_cut`` tags.
        segment_length: Interposer wire segment length.
        csv_num: Per-location wire count from the CSV (3 for 10um, 12 for 5um).

    Returns:
        None. The XML tree is modified in place.
    """
    layout_xpath = layout_xpath.rstrip("/")
    num = int(csv_num)
    half_hi, half_lo = (num + 1) // 2, num // 2
    seg_len = segment_length

    for cut in xml_root.xpath(f"{layout_xpath}/interposer_cut"):
        # Positions along a horizontal cut (y=...) are x-coordinates, and vice versa.
        dim = "W" if cut.get("y") is not None else "H"
        # The crossing cut sits at mid = dim / 2 - 1; expressions are written
        # relative to dim / 2 with the constants folded in.
        half = f"{dim} / 2"
        mid_off = -1

        for pattern_name, (_, sign) in SG_PATTERN_DIRECTIONS.items():
            wires = cut.xpath(f"./interdie_wire[@sg_name='{pattern_name}']")
            if not wires:
                continue
            if sign > 0:
                offset_start, offset_end = -(seg_len - 1), -1
                num_2, num_3 = half_hi, half_lo
            else:
                offset_start, offset_end = 1, seg_len - 1
                num_2, num_3 = half_lo, half_hi

            # Range (2) starts at mid - L + 1, so it hits mid iff L - 1 is even;
            # otherwise range (3) does. Range (4) adds the rest at mid.
            num_at_mid = num_2 if (seg_len - 1) % 2 == 0 else num_3

            # (start, end, incr, num) for each range along the cut
            ranges = [
                ("0", _offset_expr(half, mid_off - seg_len), None, num),
                (
                    _offset_expr(half, mid_off - seg_len + 1),
                    _offset_expr(half, mid_off + seg_len - 1),
                    "2",
                    num_2,
                ),
                (
                    _offset_expr(half, mid_off - seg_len + 2),
                    _offset_expr(half, mid_off + seg_len - 1),
                    "2",
                    num_3,
                ),
                (_offset_expr(half, mid_off), _offset_expr(half, mid_off), None, num - num_at_mid),
                (_offset_expr(half, mid_off + seg_len), f"{dim} - 1", None, num),
            ]

            sg_link = wires[0].get("sg_link")
            insert_idx = cut.index(wires[0])
            # Preserve the template's indentation: lxml drops an element's tail
            # (the whitespace after it) when the element is removed.
            inner_tail = wires[0].tail
            last_tail = wires[-1].tail
            for wire in wires:
                cut.remove(wire)

            new_wires = []
            for start, end, incr, range_num in ranges:
                if range_num <= 0:
                    continue
                wire = etree.Element("interdie_wire")
                wire.set("sg_name", pattern_name)
                wire.set("sg_link", sg_link)
                wire.set("offset_start", str(offset_start))
                wire.set("offset_end", str(offset_end))
                wire.set("offset_increment", "1")
                wire.set("num", str(range_num))
                wire.set("start", start)
                wire.set("end", end)
                if incr is not None:
                    wire.set("incr", incr)
                wire.tail = inner_tail
                cut.insert(insert_idx, wire)
                insert_idx += 1
                new_wires.append(wire)
            if new_wires:
                new_wires[-1].tail = last_tail


def write_arch(
    xml_tree: Any, output_dir: str, arch_id: str, gather_n_val: str, name_tag: str = ""
) -> None:
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
        name_tag: Optional tag (e.g. ``"1x4"``) inserted into the filename
            before the ``fanin`` suffix to distinguish die configurations.

    Returns:
        None. The XML file is written to ``output_dir`` and the generated path
        is printed.
    """
    tag = f"_{name_tag}" if name_tag else ""
    output_filename = f"hecate_25d_{arch_id}{tag}_fanin_{gather_n_val}.xml"
    output_path = os.path.join(output_dir, output_filename)

    xml_tree.write(output_path, encoding="UTF-8", xml_declaration=True, pretty_print=True)
    print(f"Generated: {output_path}")


def generate_archs_for_row(
    row: Mapping[str, str],
    template_path: str,
    output_dir: str,
    parser: Any,
    connection_sizes: Sequence[Tuple[str, str]],
    name_tag: str = "",
    segmented_cuts: bool = False,
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
        name_tag: Optional filename tag passed through to ``write_arch``.
        segmented_cuts: If True, use ``configure_segmented_interdie_wires``
            (four ranges along each cut) instead of ``configure_interdie_wires``.

    Returns:
        None. Generated architecture files are written to ``output_dir``.
    """
    arch_id = row["arch_id"]
    mux_name = row["mux_name"]
    wire_name = row["wire_name"]
    csv_num = row["num"]
    directionality = row["directionality"]

    # Extract the interposer segment length
    match = re.search(r"\d+", wire_name)
    segment_length = int(match.group()) if match else 1

    for gather_n_val, scatter_n_val in connection_sizes:
        # Initialize a fresh XML tree from the template for each configuration
        xml_tree = etree.parse(template_path, parser)
        xml_root = xml_tree.getroot()

        configure_interposer_segment(xml_root, segment_length, mux_name)
        configure_scatter_gather_patterns(xml_root, segment_length, gather_n_val, scatter_n_val, directionality)
        if segmented_cuts:
            configure_segmented_interdie_wires(
                xml_root, ".//layout/auto_layout/", segment_length, csv_num
            )
        else:
            configure_interdie_wires(xml_root, ".//layout/auto_layout/", segment_length, csv_num)
        write_arch(xml_tree, output_dir, arch_id, gather_n_val, name_tag)


def generate_hecate_interposer_archs(
    csv_file: str,
    template_path: str,
    output_dir: str = "hecate_25D",
    name_tag: str = "",
    segmented_cuts: bool = False,
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
        name_tag: Optional filename tag (e.g. ``"1x4"``) for generated files.
        segmented_cuts: If True, interdie wires vary along each cut (see
            ``configure_segmented_interdie_wires``). Used for the 2x2 system.

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
    gather_conn_sizes = ["4", "8", "16", "32", "64"]
    scatter_conn_sizes = ["4", "8", "16", "32", "64"]
    connection_sizes = list(zip(gather_conn_sizes, scatter_conn_sizes))

    with open(csv_file, mode="r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)

        for row in reader:
            generate_archs_for_row(
                row, template_path, output_dir, parser, connection_sizes, name_tag, segmented_cuts
            )


if __name__ == "__main__":
    # 2-die system (single interposer cut)
    generate_hecate_interposer_archs(
        "int_connectivity.csv", "hecate_25d_L18_int_10um_bump_unidir_fanin_32.xml"
    )
    # 1x4 die system (three interposer cuts), kept in its own folder
    generate_hecate_interposer_archs(
        "int_connectivity.csv",
        "hecate_25d_L18_int_10um_bump_unidir_1x4_fanin_32.xml",
        output_dir="hecate_25D_1x4",
        name_tag="1x4",
    )
    # 2x2 die system (one horizontal and one vertical interposer cut), kept in its own folder
    generate_hecate_interposer_archs(
        "int_connectivity.csv",
        "hecate_25d_L18_int_10um_bump_unidir_2x2_fanin_32.xml",
        output_dir="hecate_25D_2x2",
        name_tag="2x2",
        segmented_cuts=True,
    )
