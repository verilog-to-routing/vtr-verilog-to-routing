#!/usr/bin/python3

"""Generate Koios 3D switch-block architecture variants from a CSV description
and the base '3d_SB_koios_template_7nm.xml' architecture."""

import csv
import os
from typing import Mapping

SCATTER_GATHER_START = "  <scatter_gather_list>"
SCATTER_GATHER_END = "  </scatter_gather_list>"


def _mux_name(pitch_um: str, connectivity: str) -> str:
    prefix = "bidir_inter_die_switch" if connectivity == "bidir" else "inter_die_switch"
    return f"{prefix}_{pitch_um}um"


def _wireconn_num_conns(connectivity: str) -> str:
    return "16" if connectivity == "bidir" else "32"


def _split_conn_num(num: float) -> tuple[int, bool]:
    """Split a per-direction count into its integer and optional 0.5 fractional parts."""
    integer_part = int(num)
    has_fractional = num > integer_part
    return integer_part, has_fractional


def _indent(level: int) -> str:
    return " " * (2 * level)


def _sg_location_line(attrs: Mapping[str, str], *, everywhere: bool = False, spaced_everywhere: bool = True) -> str:
    attr_text = " ".join(f'{key}="{value}"' for key, value in attrs.items())
    if everywhere:
        everywhere_type = 'type="EVERYWHERE"   ' if spaced_everywhere else 'type="EVERYWHERE" '
        return (
            f'{_indent(3)}<sg_location {everywhere_type}'
            f'num="{attrs["num"]}" sg_link_name="{attrs["sg_link_name"]}"/>'
        )
    return f"{_indent(3)}<sg_location {attr_text}/>"


def _location_header() -> list[str]:
    return ["", f"{_indent(3)}"]


def _append_clb_locations(
    lines: list[str], pitch_um: str, clb_num: float
) -> None:
    integer_part, has_fractional = _split_conn_num(clb_num)
    clb_everywhere_spacing = pitch_um != "25"

    if integer_part > 0:
        lines.append(
            _sg_location_line(
                {"num": str(integer_part), "sg_link_name": "L_UP"},
                everywhere=True,
                spaced_everywhere=clb_everywhere_spacing,
            )
        )
        lines.append(
            _sg_location_line(
                {"num": str(integer_part), "sg_link_name": "L_DOWN"},
                everywhere=True,
                spaced_everywhere=clb_everywhere_spacing,
            )
        )

    if has_fractional:
        lines.append(
            _sg_location_line(
                {"type": "XY_SPECIFIED", "num": "1", "starty": "0", "incry": "2", "sg_link_name": "L_UP"}
            )
        )
        lines.append(
            _sg_location_line(
                {"type": "XY_SPECIFIED", "num": "1", "starty": "1", "incry": "2", "sg_link_name": "L_DOWN"}
            )
        )


def _append_dsp_locations(lines: list[str], dsp_num: float) -> None:
    integer_part, has_fractional = _split_conn_num(dsp_num)

    for index, startx in enumerate(("18", "31")):
        if integer_part > 0:
            lines.append(
                _sg_location_line(
                    {
                        "type": "XY_SPECIFIED",
                        "num": str(integer_part),
                        "startx": startx,
                        "incrx": "41",
                        "starty": "1",
                        "sg_link_name": "L_UP",
                    }
                )
            )
            lines.append(
                _sg_location_line(
                    {
                        "type": "XY_SPECIFIED",
                        "num": str(integer_part),
                        "startx": startx,
                        "incrx": "41",
                        "starty": "1",
                        "sg_link_name": "L_DOWN",
                    }
                )
            )

        if has_fractional:
            lines.append(
                _sg_location_line(
                    {
                        "type": "XY_SPECIFIED",
                        "num": "1",
                        "startx": startx,
                        "incrx": "41",
                        "starty": "1",
                        "incry": "2",
                        "sg_link_name": "L_UP",
                    }
                )
            )
            lines.append(
                _sg_location_line(
                    {
                        "type": "XY_SPECIFIED",
                        "num": "1",
                        "startx": startx,
                        "incrx": "41",
                        "starty": "2",
                        "incry": "2",
                        "sg_link_name": "L_DOWN",
                    }
                )
            )

        if index == 0:
            lines.append("")


def _append_mem_locations(lines: list[str], mem_num: float) -> None:
    integer_part, has_fractional = _split_conn_num(mem_num)

    for index, startx in enumerate(("11", "25", "37")):
        if integer_part > 0:
            lines.append(
                _sg_location_line(
                    {
                        "type": "XY_SPECIFIED",
                        "num": str(integer_part),
                        "startx": startx,
                        "incrx": "41",
                        "starty": "1",
                        "sg_link_name": "L_UP",
                    }
                )
            )
            lines.append(
                _sg_location_line(
                    {
                        "type": "XY_SPECIFIED",
                        "num": str(integer_part),
                        "startx": startx,
                        "incrx": "41",
                        "starty": "1",
                        "sg_link_name": "L_DOWN",
                    }
                )
            )

        if has_fractional:
            lines.append(
                _sg_location_line(
                    {
                        "type": "XY_SPECIFIED",
                        "num": "1",
                        "startx": startx,
                        "incrx": "41",
                        "starty": "1",
                        "incry": "2",
                        "sg_link_name": "L_UP",
                    }
                )
            )
            lines.append(
                _sg_location_line(
                    {
                        "type": "XY_SPECIFIED",
                        "num": "1",
                        "startx": startx,
                        "incrx": "41",
                        "starty": "2",
                        "incry": "2",
                        "sg_link_name": "L_DOWN",
                    }
                )
            )

        if index != 2:
            lines.append("")


def _build_locations(
    pitch_um: str, clb_num: float, dsp_num: float, mem_num: float
) -> list[str]:
    clb_total = int(clb_num * 2)
    dsp_total = clb_total + int(dsp_num * 2)
    mem_total = clb_total + int(mem_num * 2)

    lines = [
        *_location_header(),
        f"{_indent(3)}<!-- {clb_total} connections per CLB  -->",
    ]
    _append_clb_locations(lines, pitch_um, clb_num)
    lines.extend(
        [
            "",
            (
                f"{_indent(3)}<!-- {dsp_total} Connections per DSP. "
                f"{clb_total} of them are already covered by CLB connections. -->"
            ),
        ]
    )
    _append_dsp_locations(lines, dsp_num)
    lines.extend(
        [
            "",
            (
                f"{_indent(3)}<!-- {mem_total} Connections per Memory. "
                f"{clb_total} of them are already covered by CLB connections. -->"
            ),
        ]
    )
    _append_mem_locations(lines, mem_num)
    return lines


def build_scatter_gather_section(row: Mapping[str, str]) -> str:
    """Build the scatter_gather_list XML section for one Koios variant."""
    pitch_um = row["pitch_um"]
    connectivity = row["connectivity"]
    clb_num = float(row["clb_num"])
    dsp_num = float(row["dsp_num"])
    mem_num = float(row["mem_num"])

    assert clb_num * 2 == int(clb_num * 2), f"clb_num * 2 must be an integer, got {clb_num}"
    assert dsp_num * 2 == int(dsp_num * 2), f"dsp_num * 2 must be an integer, got {dsp_num}"
    assert mem_num * 2 == int(mem_num * 2), f"mem_num * 2 must be an integer, got {mem_num}"

    mux_name = _mux_name(pitch_um, connectivity)
    wireconn_num = _wireconn_num_conns(connectivity)

    lines = [
        SCATTER_GATHER_START,
        f'{_indent(2)}<sg_pattern name="3d_sb_sg_10um_dir" type="{connectivity}">',
        f"{_indent(3)}<gather>",
        (
            f'{_indent(4)}<wireconn num_conns="{wireconn_num}" from_type="L4" '
            'from_switchpoint="0,1,2,3" side="rltb"/>'
        ),
        f"{_indent(3)}</gather>",
        f"{_indent(3)}<scatter>",
        (
            f'{_indent(4)}<wireconn num_conns="{wireconn_num}" to_type="L4" '
            'to_switchpoint="0" side="rtlb"/>'
        ),
        f"{_indent(3)}</scatter>",
        f"{_indent(3)}<sg_link_list>",
        (
            f'{_indent(4)}<sg_link name="L_UP"   z_offset="1"  '
            f'mux="{mux_name}" seg_type="z_segment"/>'
        ),
        (
            f'{_indent(4)}<sg_link name="L_DOWN" z_offset="-1" '
            f'mux="{mux_name}" seg_type="z_segment"/>'
        ),
        f"{_indent(3)}</sg_link_list>",
    ]

    lines.extend(_build_locations(pitch_um, clb_num, dsp_num, mem_num))

    if row["variant_id"] == "10um_reduced_bidir":
        lines.extend(["", f"{_indent(2)}</sg_pattern>", SCATTER_GATHER_END])
    else:
        lines.extend(["", f"{_indent(2)}</sg_pattern>", "", SCATTER_GATHER_END])

    return "\n".join(lines)


def _replace_xml_section(template_text: str, start_tag: str, end_tag: str, new_content: str) -> str:
    """Replace the content between two XML tags while preserving surrounding text."""
    start_idx = template_text.index(start_tag)
    end_idx = template_text.index(end_tag, start_idx) + len(end_tag)
    return template_text[:start_idx] + new_content + template_text[end_idx:]


def generate_koios_arch_for_row(row: Mapping[str, str], template_text: str, output_dir: str) -> None:
    """Generate one Koios 3D switch-block architecture variant."""
    variant_id = row["variant_id"]
    scatter_gather_section = build_scatter_gather_section(row)
    arch_text = _replace_xml_section(
        template_text, SCATTER_GATHER_START, SCATTER_GATHER_END, scatter_gather_section
    )

    output_filename = f"3d_SB_koios_{variant_id}_7nm.xml"
    output_path = os.path.join(output_dir, output_filename)

    with open(output_path, "w", encoding="utf-8") as output_file:
        output_file.write(arch_text)

    print(f"Generated: {output_path}")


def generate_koios_3d_sb_archs(
    csv_file: str, template_path: str, output_dir: str = "hecate_3D"
) -> None:
    """Generate Koios 3D switch-block architecture XML variants."""
    if not os.path.exists(template_path):
        print(f"Error: Template {template_path} not found.")
        return

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    with open(template_path, encoding="utf-8") as template_file:
        template_text = template_file.read()

    with open(csv_file, mode="r", newline="", encoding="utf-8") as csv_handle:
        reader = csv.DictReader(csv_handle)
        for row in reader:
            generate_koios_arch_for_row(row, template_text, output_dir)


if __name__ == "__main__":
    generate_koios_3d_sb_archs("koios_sb_connectivity.csv", "3d_SB_koios_template_7nm.xml")
