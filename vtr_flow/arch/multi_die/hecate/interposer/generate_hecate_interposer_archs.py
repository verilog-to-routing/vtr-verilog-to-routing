#!/usr/bin/python3

"""Generate Hecate interposer architecture variants from CSV/JSON descriptions
and the base Hecate architectures:
  - 'hecate_25d_L18_int_10um_bump_unidir_fanin_32.xml' (2 dies, 1 cut)
  - 'hecate_25d_L18_int_10um_bump_unidir_1x4_fanin_32.xml' (1x4 dies, 3 cuts)
  - 'hecate_25d_L18_int_10um_bump_unidir_2x2_fanin_32.xml' (2x2 dies, 2 cuts)
"""

import copy
import csv
import json
import os
import re
from typing import Any, List, Mapping, NamedTuple, Optional, Sequence, Tuple
from lxml import etree

# pylint: disable=c-extension-no-member

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
VALID_DIRECTIONALITIES = ("unidir", "bidir")
BUMP_PITCH_TO_NUM = {"10um": "3", "5um": "12"}
DEFAULT_FAN_IN_SIZES = ("4", "8", "16", "32", "64")

# Scatter-gather pattern name -> (sg_link offset attribute, offset sign).
# Up/down patterns cross horizontal cuts; left/right patterns cross vertical cuts.
SG_PATTERN_DIRECTIONS = {
    "interposer_sg_downward": ("y_offset", -1),
    "interposer_sg_upward": ("y_offset", 1),
    "interposer_sg_leftward": ("x_offset", -1),
    "interposer_sg_rightward": ("x_offset", 1),
}

# Variable-length 2-die scatter-gather pattern name -> (sg_link prefix, y_offset sign).
VARLEN_SG_DIRECTIONS = {
    "interposer_sg_downward": ("DOWN", -1),
    "interposer_sg_upward": ("UP", 1),
}


class ArchGenConfig(NamedTuple):
    """Shared settings for generating fixed-length variants from one template."""

    base_tree: Any
    output_dir: str
    connection_sizes: Sequence[Tuple[str, str]]
    name_tag: str = ""
    segmented_cuts: bool = False


# ---------------------------------------------------------------------------
# Shared helpers
# ---------------------------------------------------------------------------


def _validate_directionality(directionality: str) -> None:
    if directionality not in VALID_DIRECTIONALITIES:
        raise ValueError(
            f"Invalid directionality '{directionality}'; "
            f"expected one of {VALID_DIRECTIONALITIES}."
        )


def _parse_segment_length(wire_name: str) -> int:
    """Extract and validate the interposer segment length from ``wire_name``."""
    match = re.search(r"\d+", wire_name)
    if not match:
        raise ValueError(f"Could not extract segment length from wire_name '{wire_name}'.")
    segment_length = int(match.group())
    if segment_length < 2:
        raise ValueError(
            f"Interposer segment length must be >= 2 (got {segment_length} from '{wire_name}')."
        )
    return segment_length


def _load_template_tree(template_path: str) -> Any:
    """Parse an architecture XML template while preserving comments."""
    parser = etree.XMLParser(remove_comments=False)
    return etree.parse(template_path, parser)


def _set_sg_wireconn_conns(
    sg: Any, pattern_name: str, gather_n_val: str, scatter_n_val: str
) -> None:
    """Set gather and scatter ``wireconn/@num_conns`` on an ``sg_pattern`` tag."""
    for tag_name, n_val in (("gather", gather_n_val), ("scatter", scatter_n_val)):
        wireconn = sg.xpath(f"./{tag_name}/wireconn")
        if not wireconn:
            raise ValueError(
                f"Missing <{tag_name}><wireconn> tag in sg_pattern '{pattern_name}'."
            )
        wireconn[0].set("num_conns", n_val)


def _write_xml(xml_tree: Any, output_path: str) -> None:
    """Serialize an architecture XML tree to ``output_path``."""
    xml_tree.write(output_path, encoding="UTF-8", xml_declaration=True, pretty_print=True)
    print(f"Generated: {output_path}")


# ---------------------------------------------------------------------------
# Fixed-length interposer architecture generation
# ---------------------------------------------------------------------------


def configure_interposer_segment(xml_root: Any, segment_length: int, mux_name: str) -> None:
    """Configure the physical interposer segment definition.

    Updates the ``int_wire`` segment in the architecture XML so that its
    length, mux name, switch-block pattern, and connection-block pattern match
    the interposer wire described by the current CSV row.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        segment_length: Length to assign to the ``int_wire`` segment.
        mux_name: Name to assign to the segment's nested ``mux`` tag.

    Raises:
        ValueError: If the ``int_wire`` segment or any of its required child
            tags (``mux``, ``sb``, ``cb``) is missing from the template.
    """
    segment = xml_root.xpath(".//segmentlist/segment[@name='int_wire']")
    if not segment:
        raise ValueError("Missing 'int_wire' segment in template XML.")

    seg = segment[0]
    seg.set("length", str(segment_length))

    mux_tag = seg.find("mux")
    if mux_tag is None:
        raise ValueError("Missing <mux> tag under 'int_wire' segment.")
    mux_tag.set("name", mux_name)

    # Pattern logic: sb = L+1, cb = L
    sb_tag = seg.xpath("./sb[@type='pattern']")
    if not sb_tag:
        raise ValueError("Missing <sb type='pattern'> tag under 'int_wire' segment.")
    sb_tag[0].text = " ".join(["0"] * (segment_length + 1))

    cb_tag = seg.xpath("./cb[@type='pattern']")
    if not cb_tag:
        raise ValueError("Missing <cb type='pattern'> tag under 'int_wire' segment.")
    cb_tag[0].text = " ".join(["0"] * segment_length)


def configure_scatter_gather_patterns(
    xml_root: Any,
    segment_length: int,
    gather_n_val: str,
    scatter_n_val: str,
    directionality: str,
) -> None:
    """Configure interposer scatter-gather connectivity patterns.

    Updates the up/down/left/right interposer scatter-gather patterns
    (whichever exist in the template) so their directionality, gather/scatter
    connection counts, and scatter-gather link offsets match the generated
    architecture variant.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        segment_length: Interposer wire segment length. This determines the
            signed ``y_offset`` (up/down) or ``x_offset`` (left/right) used by
            each scatter-gather link.
        gather_n_val: Number of gather connections to write to each matching
            ``gather/wireconn`` tag.
        scatter_n_val: Number of scatter connections to write to each matching
            ``scatter/wireconn`` tag.
        directionality: Interposer link directionality (``"unidir"`` or
            ``"bidir"``).

    Raises:
        ValueError: If ``directionality`` is invalid, if no interposer
            ``sg_pattern`` tags exist in the template, or if a matched pattern
            is missing required child tags.
    """
    _validate_directionality(directionality)

    matched = False
    for pattern_name, (offset_attr, sign) in SG_PATTERN_DIRECTIONS.items():
        sg_list = xml_root.xpath(
            f".//scatter_gather_list/sg_pattern[@name='{pattern_name}']"
        )
        if not sg_list:
            continue
        matched = True
        sg = sg_list[0]
        sg.set("type", directionality)

        # Update num_conns for gather and scatter (4, 8, 16, 32, or 64)
        _set_sg_wireconn_conns(sg, pattern_name, gather_n_val, scatter_n_val)

        # Update the sg_link X/Y offset depending on the interposer routing direction
        sg_link = sg.xpath("./sg_link_list/sg_link")
        if not sg_link:
            raise ValueError(f"Missing <sg_link> tag in sg_pattern '{pattern_name}'.")
        sg_link[0].set(offset_attr, str(sign * segment_length))

    if not matched:
        raise ValueError("No interposer <sg_pattern> tags found in template XML.")


def configure_interdie_wires(
    xml_root: Any, layout_xpath: str, segment_length: int, csv_num: str
) -> None:
    """Configure interposer cut definitions for a layout.

    Updates the up/down/left/right ``interdie_wire`` tags under every
    ``interposer_cut`` of the selected layout (e.g. one cut for a 2-die
    system, three cuts for a 1x4 system, one horizontal and one vertical cut
    for a 2x2 system). Wires going up/right start ``segment_length - 1``
    tiles before the cut; wires going down/left start just after it. The
    layout is selected by XPath so the same logic can be reused for any
    layout tag.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        layout_xpath: XPath selecting the layout element to update, such as
            ``.//layout/auto_layout/``. A trailing slash is optional.
        segment_length: Interposer wire segment length. This determines the
            offset values of the interdie_wire tag.
        csv_num: Number of inter-die wires instantiated at each switchblock
            location. Determines the ``num`` value of the interdie_wire tag.

    Raises:
        ValueError: If no matching ``interdie_wire`` tags are found under
            ``layout_xpath``.
    """
    layout_xpath = layout_xpath.rstrip("/")
    updated = False
    for pattern_name, (_, sign) in SG_PATTERN_DIRECTIONS.items():
        if sign > 0:
            offset_start, offset_end = -(segment_length - 1), -1
        else:
            offset_start, offset_end = 1, segment_length - 1
        for wire in xml_root.xpath(
            f"{layout_xpath}/interposer_cut/interdie_wire[@sg_name='{pattern_name}']"
        ):
            updated = True
            wire.set("offset_start", str(offset_start))
            wire.set("offset_end", str(offset_end))
            wire.set("num", csv_num)

    if not updated:
        raise ValueError(f"No matching <interdie_wire> tags found under '{layout_xpath}'.")


def _offset_expr(base: str, delta: int) -> str:
    """Return a grid expression string for ``base + delta`` (e.g. ``"W / 2 - 19"``)."""
    if delta == 0:
        return base
    return f"{base} {'+' if delta > 0 else '-'} {abs(delta)}"


def _compute_segmented_ranges(
    dim: str, seg_len: int, num: int, sign: int
) -> List[Tuple[str, str, Optional[str], int]]:
    """Compute the five ``(start, end, incr, num)`` ranges along a crossing cut."""
    half_hi, half_lo = (num + 1) // 2, num // 2
    num_2, num_3 = (half_hi, half_lo) if sign > 0 else (half_lo, half_hi)

    # The crossing cut sits at mid = dim / 2 - 1; expressions are written
    # relative to dim / 2 with the constants folded in.
    half = f"{dim} / 2"
    mid_off = -1

    # Range (2) starts at mid - L + 1, so it hits mid iff L - 1 is even;
    # otherwise range (3) does. Range (4) adds the rest at mid.
    num_at_mid = num_2 if (seg_len - 1) % 2 == 0 else num_3

    return [
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


def _resolve_wire_inner_tail(cut: Any, wires: Sequence[Any]) -> Optional[str]:
    """Return the whitespace tail to place between inserted ``interdie_wire`` tags."""
    if len(wires) > 1:
        return wires[0].tail
    # For a single template tag (possibly the last child of <interposer_cut>),
    # wires[0].tail may be the closing </interposer_cut> indent rather than the
    # child indent, so derive the child indent from cut.text instead.
    if cut.text and "\n" in cut.text:
        return "\n" + cut.text.rsplit("\n", 1)[-1]
    return wires[0].tail


def _build_interdie_wire(
    pattern_name: str,
    sg_link: str,
    offsets: Tuple[int, int],
    range_spec: Tuple[str, str, Optional[str], int],
) -> Any:
    """Create one segmented ``<interdie_wire>`` XML element."""
    start, end, incr, range_num = range_spec
    wire = etree.Element("interdie_wire")
    wire.set("sg_name", pattern_name)
    wire.set("sg_link", sg_link)
    wire.set("offset_start", str(offsets[0]))
    wire.set("offset_end", str(offsets[1]))
    wire.set("offset_increment", "1")
    wire.set("num", str(range_num))
    wire.set("start", start)
    wire.set("end", end)
    if incr is not None:
        wire.set("incr", incr)
    return wire


def _replace_cut_direction_wires(
    cut: Any, pattern_name: str, sign: int, seg_len: int, num: int
) -> bool:
    """Replace one direction's ``interdie_wire`` tags under ``cut`` with segmented ranges."""
    wires = cut.xpath(f"./interdie_wire[@sg_name='{pattern_name}']")
    if not wires:
        return False

    sg_link = wires[0].get("sg_link")
    if not sg_link:
        raise ValueError(f"Missing 'sg_link' attribute on <interdie_wire> for '{pattern_name}'.")

    # Positions along a horizontal cut (y=...) are x-coordinates, and vice versa.
    dim = "W" if cut.get("y") is not None else "H"
    offsets = (-(seg_len - 1), -1) if sign > 0 else (1, seg_len - 1)

    insert_idx = cut.index(wires[0])
    # Preserve the template's indentation: lxml drops an element's tail
    # (the whitespace after it) when the element is removed.
    inner_tail = _resolve_wire_inner_tail(cut, wires)
    last_tail = wires[-1].tail
    for wire in wires:
        cut.remove(wire)

    new_wires = [
        _build_interdie_wire(pattern_name, sg_link, offsets, range_spec)
        for range_spec in _compute_segmented_ranges(dim, seg_len, num, sign)
        if range_spec[3] > 0
    ]
    for offset_idx, wire in enumerate(new_wires):
        wire.tail = inner_tail
        cut.insert(insert_idx + offset_idx, wire)

    if new_wires:
        new_wires[-1].tail = last_tail
    return True


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

    For each direction under each cut, the template ``interdie_wire`` tag(s)
    are replaced by five tags along the cut (``mid`` is the position of the
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

    Raises:
        ValueError: If no ``interposer_cut`` or matching ``interdie_wire`` tags
            are found under ``layout_xpath``.
    """
    layout_xpath = layout_xpath.rstrip("/")
    num = int(csv_num)
    cuts = xml_root.xpath(f"{layout_xpath}/interposer_cut")
    if not cuts:
        raise ValueError(f"No <interposer_cut> tags found under '{layout_xpath}'.")

    updated = False
    for cut in cuts:
        for pattern_name, (_, sign) in SG_PATTERN_DIRECTIONS.items():
            if _replace_cut_direction_wires(cut, pattern_name, sign, segment_length, num):
                updated = True

    if not updated:
        raise ValueError(f"No matching <interdie_wire> tags found under '{layout_xpath}'.")


def write_arch(
    xml_tree: Any, output_dir: str, arch_id: str, gather_n_val: str, name_tag: str = ""
) -> None:
    """Write a generated fixed-length architecture XML file to disk.

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
    """
    tag = f"_{name_tag}" if name_tag else ""
    output_filename = f"hecate_25d_{arch_id}{tag}_fanin_{gather_n_val}.xml"
    _write_xml(xml_tree, os.path.join(output_dir, output_filename))


def generate_archs_for_row(row: Mapping[str, str], config: ArchGenConfig) -> None:
    """Generate all fixed-length architecture variants described by one CSV row.

    Args:
        row: CSV row containing ``arch_id``, ``mux_name``, ``wire_name``,
            ``num``, and ``directionality`` fields.
        config: Shared generation settings (parsed template tree, output
            directory, connection sizes, optional filename tag, and whether to
            use five-range segmented cuts).
    """
    segment_length = _parse_segment_length(row["wire_name"])

    for gather_n_val, scatter_n_val in config.connection_sizes:
        xml_tree = copy.deepcopy(config.base_tree)
        xml_root = xml_tree.getroot()

        configure_interposer_segment(xml_root, segment_length, row["mux_name"])
        configure_scatter_gather_patterns(
            xml_root, segment_length, gather_n_val, scatter_n_val, row["directionality"]
        )
        if config.segmented_cuts:
            configure_segmented_interdie_wires(
                xml_root, ".//layout/auto_layout/", segment_length, row["num"]
            )
        else:
            configure_interdie_wires(xml_root, ".//layout/auto_layout/", segment_length, row["num"])
        write_arch(xml_tree, config.output_dir, row["arch_id"], gather_n_val, config.name_tag)


def generate_hecate_interposer_archs(
    csv_file: str,
    template_path: str,
    output_dir: str = "hecate_25D",
    name_tag: str = "",
    segmented_cuts: bool = False,
) -> None:
    """Generate fixed-length Hecate interposer architecture XML variants.

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
        segmented_cuts: If True, interdie wires vary along each cut in five
            ranges (see ``configure_segmented_interdie_wires``). Used for the
            2x2 system.

    Raises:
        FileNotFoundError: If ``template_path`` or ``csv_file`` does not exist.
    """
    if not os.path.exists(template_path):
        raise FileNotFoundError(f"Template '{template_path}' not found.")
    if not os.path.exists(csv_file):
        raise FileNotFoundError(f"CSV file '{csv_file}' not found.")

    os.makedirs(output_dir, exist_ok=True)
    base_tree = _load_template_tree(template_path)
    connection_sizes = list(zip(DEFAULT_FAN_IN_SIZES, DEFAULT_FAN_IN_SIZES))

    config = ArchGenConfig(
        base_tree=base_tree,
        output_dir=output_dir,
        connection_sizes=connection_sizes,
        name_tag=name_tag,
        segmented_cuts=segmented_cuts,
    )

    with open(csv_file, mode="r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            generate_archs_for_row(row, config)


# ---------------------------------------------------------------------------
# Variable-length interposer architecture generation (2-die)
# ---------------------------------------------------------------------------


def configure_varlen_interdie_wires(
    xml_root: Any, layout_xpath: str, lengths: Sequence[int], wire_um: str
) -> None:
    """Replace ``interdie_wire`` tags with one centered up/down pair per length.

    Each wire of length ``L`` is placed at offset ``-L/2`` (upward) and
    ``+L/2`` (downward) from the cut so that every wire straddles the cut
    symmetrically.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        layout_xpath: XPath selecting the layout element, e.g.
            ``".//layout/auto_layout"``.
        lengths: Interposer wire segment lengths (each must be even and >= 2).
        wire_um: Bump pitch (``"10um"`` or ``"5um"``), determining ``num``.

    Raises:
        ValueError: If ``wire_um`` is invalid, any length is odd, or no
            ``interposer_cut`` exists under ``layout_xpath``.
    """
    if wire_um not in BUMP_PITCH_TO_NUM:
        raise ValueError(
            f"Invalid wire_um '{wire_um}'; expected one of {tuple(BUMP_PITCH_TO_NUM)}."
        )
    layout_xpath = layout_xpath.rstrip("/")
    cuts = xml_root.xpath(f"{layout_xpath}/interposer_cut")
    if not cuts:
        raise ValueError(f"No <interposer_cut> tags found under '{layout_xpath}'.")
    cut = cuts[0]

    for wire in cut.findall("interdie_wire"):
        cut.remove(wire)

    num = BUMP_PITCH_TO_NUM[wire_um]
    for length in lengths:
        if length % 2 != 0:
            raise ValueError(f"Variable-length interposer wire length must be even (got {length}).")
        offset = str(length // 2)
        etree.SubElement(
            cut,
            "interdie_wire",
            {
                "sg_name": "interposer_sg_upward",
                "sg_link": f"sg_UP_L{length}",
                "offset_start": f"-{offset}",
                "offset_end": f"-{offset}",
                "offset_increment": "1",
                "num": num,
            },
        )
        etree.SubElement(
            cut,
            "interdie_wire",
            {
                "sg_name": "interposer_sg_downward",
                "sg_link": f"sg_DOWN_L{length}",
                "offset_start": offset,
                "offset_end": offset,
                "offset_increment": "1",
                "num": num,
            },
        )


def configure_varlen_switches(
    xml_root: Any,
    length_delays: Sequence[Tuple[int, Sequence[float]]],
    wire_um: str,
    directionality: str,
    fan_in_values: Sequence[str],
) -> None:
    """Replace ``int_driver`` switches with per-length variable-length drivers.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        length_delays: Sequence of ``(length, delays_ps)`` pairs where each
            ``delays_ps`` sequence aligns with ``fan_in_values``.
        wire_um: Bump pitch (``"10um"`` or ``"5um"``), used in the switch name.
        directionality: ``"unidir"`` or ``"bidir"`` (``bidir`` doubles
            ``num_inputs``).
        fan_in_values: Fan-in labels matching the order of each delay list.

    Raises:
        ValueError: If ``directionality`` is invalid, ``switchlist`` is
            missing, or a delay list length does not match ``fan_in_values``.
    """
    _validate_directionality(directionality)
    switchlists = xml_root.xpath(".//switchlist")
    if not switchlists:
        raise ValueError("Missing <switchlist> tag in template XML.")
    switchlist = switchlists[0]

    for switch in switchlist.xpath("./switch[contains(@name, 'int_driver')]"):
        switchlist.remove(switch)

    for length, delays in length_delays:
        if len(delays) != len(fan_in_values):
            raise ValueError(
                f"Expected {len(fan_in_values)} delays for L{length}, got {len(delays)}."
            )
        switch = etree.SubElement(
            switchlist,
            "switch",
            {
                "type": "mux",
                "name": f"L{length}_int_driver_{wire_um}_bump",
                "R": "0.0",
                "Cout": "0.0",
                "Cin": "0.0",
                "mux_trans_size": "0",
                "buf_size": "0",
            },
        )
        for fan_in, delay in zip(fan_in_values, delays):
            num_inputs = str(int(fan_in) * 2) if directionality == "bidir" else fan_in
            etree.SubElement(switch, "Tdel", {"num_inputs": num_inputs, "delay": f"{delay}e-12"})


def configure_varlen_segments(
    xml_root: Any, lengths: Sequence[int], wire_um: str
) -> None:
    """Replace ``int_wire`` with one ``int_wire_L{length}`` segment per length.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        lengths: Interposer wire segment lengths to create.
        wire_um: Bump pitch (``"10um"`` or ``"5um"``), used in the mux name.

    Raises:
        ValueError: If ``segmentlist`` is missing from the template.
    """
    segmentlists = xml_root.xpath(".//segmentlist")
    if not segmentlists:
        raise ValueError("Missing <segmentlist> tag in template XML.")
    segmentlist = segmentlists[0]

    for segment in segmentlist.xpath("./segment[@name='int_wire']"):
        segmentlist.remove(segment)

    for length in lengths:
        segment = etree.SubElement(
            segmentlist,
            "segment",
            {
                "name": f"int_wire_L{length}",
                "freq": "0",
                "length": str(length),
                "type": "unidir",
                "Rmetal": "0.0",
                "Cmetal": "0.0",
            },
        )
        etree.SubElement(segment, "mux", {"name": f"L{length}_int_driver_{wire_um}_bump"})
        etree.SubElement(segment, "sb", {"type": "pattern"}).text = " ".join(["0"] * (length + 1))
        etree.SubElement(segment, "cb", {"type": "pattern"}).text = " ".join(["0"] * length)


def configure_varlen_scatter_gather(
    xml_root: Any, lengths: Sequence[int], fan_in: str, directionality: str
) -> None:
    """Configure upward/downward scatter-gather patterns for variable-length wires.

    Args:
        xml_root: Root element of the parsed architecture XML tree.
        lengths: Interposer wire segment lengths to emit as ``sg_link`` entries.
        fan_in: Gather/scatter ``num_conns`` value (not doubled for ``bidir``).
        directionality: ``"unidir"`` or ``"bidir"``.

    Raises:
        ValueError: If ``directionality`` is invalid or required scatter-gather
            tags are missing from the template.
    """
    _validate_directionality(directionality)

    for sg_name, (link_prefix, sign) in VARLEN_SG_DIRECTIONS.items():
        patterns = xml_root.xpath(f".//scatter_gather_list/sg_pattern[@name='{sg_name}']")
        if not patterns:
            raise ValueError(f"Missing <sg_pattern name='{sg_name}'> in template XML.")
        sg = patterns[0]
        sg.set("type", directionality)
        _set_sg_wireconn_conns(sg, sg_name, fan_in, fan_in)

        sg_link_lists = sg.xpath("./sg_link_list")
        if not sg_link_lists:
            raise ValueError(f"Missing <sg_link_list> in sg_pattern '{sg_name}'.")
        sg_link_list = sg_link_lists[0]

        for sg_link in sg_link_list.findall("sg_link"):
            sg_link_list.remove(sg_link)

        for length in lengths:
            etree.SubElement(
                sg_link_list,
                "sg_link",
                {
                    "name": f"sg_{link_prefix}_L{length}",
                    "y_offset": str(sign * length),
                    "seg_type": f"int_wire_L{length}",
                },
            )


def _generate_varlen_group(
    base_tree: Any,
    output_dir: str,
    max_length: str,
    layers: Mapping[str, Any],
    fan_in_values: Sequence[str],
) -> None:
    """Generate all variable-length variants for one ``Max_L*`` group."""
    for directionality in VALID_DIRECTIONALITIES:
        for wire_um in BUMP_PITCH_TO_NUM:
            length_delays = [
                (_parse_segment_length(l_key), wire_data[wire_um][directionality])
                for l_key, wire_data in layers.items()
            ]
            lengths = [length for length, _ in length_delays]

            # Switches, segments, and interdie_wires are shared across fan-in values.
            group_tree = copy.deepcopy(base_tree)
            group_root = group_tree.getroot()
            configure_varlen_interdie_wires(group_root, ".//layout/auto_layout", lengths, wire_um)
            configure_varlen_switches(
                group_root, length_delays, wire_um, directionality, fan_in_values
            )
            configure_varlen_segments(group_root, lengths, wire_um)

            for fan_in in fan_in_values:
                xml_tree = copy.deepcopy(group_tree)
                configure_varlen_scatter_gather(
                    xml_tree.getroot(), lengths, fan_in, directionality
                )
                etree.indent(xml_tree, space="  ")

                output_filename = (
                    f"hecate_25d_{max_length}_{directionality}_{wire_um}_fanin_{fan_in}.xml"
                )
                _write_xml(xml_tree, os.path.join(output_dir, output_filename))


def generate_varlen_hecate_interposer_archs(
    json_file: str,
    template_path: str,
    output_dir: str = "hecate_varlen_25D",
) -> None:
    """Generate variable-length 2-die Hecate interposer architecture XML variants.

    Reads a JSON description of per-``Max_L*`` segment delay tables and applies
    each combination of ``(max_length, directionality, bump_pitch, fan_in)`` to
    the base 2-die Hecate architecture template.

    Args:
        json_file: Path to the JSON file containing ``fan_in_values`` and
            ``arches`` delay tables.
        template_path: Path to the base 2-die Hecate architecture XML template.
        output_dir: Directory where generated architecture XML files should be
            written. Created if it does not already exist.

    Raises:
        FileNotFoundError: If ``template_path`` or ``json_file`` does not exist.
    """
    if not os.path.exists(template_path):
        raise FileNotFoundError(f"Template '{template_path}' not found.")
    if not os.path.exists(json_file):
        raise FileNotFoundError(f"JSON file '{json_file}' not found.")

    with open(json_file, mode="r", encoding="utf-8") as f:
        data = json.load(f)

    os.makedirs(output_dir, exist_ok=True)
    base_tree = _load_template_tree(template_path)
    fan_in_values = data["fan_in_values"]

    for max_length, layers in data["arches"].items():
        _generate_varlen_group(base_tree, output_dir, max_length, layers, fan_in_values)


if __name__ == "__main__":
    csv_path = os.path.join(SCRIPT_DIR, "int_connectivity.csv")
    varlen_json_path = os.path.join(SCRIPT_DIR, "int_varlen_connectivity.json")
    two_die_template = os.path.join(
        SCRIPT_DIR, "hecate_25d_L18_int_10um_bump_unidir_fanin_32.xml"
    )

    # 2-die system (single interposer cut)
    generate_hecate_interposer_archs(
        csv_path,
        two_die_template,
        output_dir=os.path.join(SCRIPT_DIR, "hecate_25D"),
    )
    # 1x4 die system (three interposer cuts), kept in its own folder
    generate_hecate_interposer_archs(
        csv_path,
        os.path.join(SCRIPT_DIR, "hecate_25d_L18_int_10um_bump_unidir_1x4_fanin_32.xml"),
        output_dir=os.path.join(SCRIPT_DIR, "hecate_25D_1x4"),
        name_tag="1x4",
    )
    # 2x2 die system (one horizontal and one vertical interposer cut), kept in its own folder
    generate_hecate_interposer_archs(
        csv_path,
        os.path.join(SCRIPT_DIR, "hecate_25d_L18_int_10um_bump_unidir_2x2_fanin_32.xml"),
        output_dir=os.path.join(SCRIPT_DIR, "hecate_25D_2x2"),
        name_tag="2x2",
        segmented_cuts=True,
    )
    # Variable-length 2-die system (single interposer cut with multiple wire lengths)
    generate_varlen_hecate_interposer_archs(
        varlen_json_path,
        two_die_template,
        output_dir=os.path.join(SCRIPT_DIR, "hecate_varlen_25D"),
    )
