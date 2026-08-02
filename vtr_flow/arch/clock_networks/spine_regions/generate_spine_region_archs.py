#!/usr/bin/python3

"""Generate NxN disjoint-quadrant clock network architecture variants from the
base 'k6_frac_N10_frac_chain_mem32K_clk_2_by_2.xml' architecture.

Each variant divides the device into an NxN grid of quadrants. A single
sparse clock_switch_grid at the top of the clock hierarchy drives one spine
and one rib per quadrant, each quadrant's rib feeding the CLOCK pins within
its own region. See the 2x2 architecture for the hand-written version this
pattern is based on.

All clocknetworks coordinates are expressed as multiples of a single base
unit, W/(2N) and H/(2N), so that spine/rib boundaries and clock_switch_grid
tap locations always land on exactly the same grid points regardless of
device size -- mixing units (e.g. W/4 alongside W/8) can round to different
physical positions for non-power-of-two device dimensions.
"""

import os
from typing import Any
from lxml import etree

# pylint: disable=c-extension-no-member

TEMPLATE_NAME = "k6_frac_N10_frac_chain_mem32K_clk_2_by_2.xml"
OUTPUT_NAME_FMT = "k6_frac_N10_frac_chain_mem32K_clk_{n}_by_{n}.xml"


def frac(k: int, var: str, denom: int) -> str:
    """Format k*(var/denom), simplified for the k=0 and k=1 cases.

    Args:
        k: Integer coefficient.
        var: Device dimension variable, either "W" or "H".
        denom: Base unit denominator, e.g. 16 for an 8x8 grid.

    Returns:
        An architecture formula string, e.g. "3*(W/16)".
    """
    if k == 0:
        return "0"
    if k == 1:
        return f"{var}/{denom}"
    return f"{k}*({var}/{denom})"


def comment(text: str) -> etree._Comment:
    """Create an XML comment element with a leading/trailing space."""
    return etree.Comment(f" {text} ")


def qname(col: int, row: int) -> str:
    """Format a quadrant identifier for use in switch_point/clock_network names.

    Uses an explicit separator between col and row so identifiers stay
    unambiguous once either index reaches two digits (N >= 10): without a
    separator, col=1/row=32 and col=13/row=2 would both stringify to "132",
    silently aliasing two different switch points onto the same name.
    """
    return f"{col}_{row}"


def build_switch_grid(n: int, denom: int) -> Any:
    """Build the top-of-hierarchy clock_switch_grid clock_network element.

    Args:
        n: Grid size (the network divides the device into an NxN quadrant grid).
        denom: Base unit denominator, 2*n.

    Returns:
        The <clock_network name="global_clk_switch_network" ...> element,
        containing the drive point at the device center and one tap per
        quadrant center.
    """
    network = etree.Element("clock_network", name="global_clk_switch_network", num_inst="1")
    grid = etree.SubElement(
        network,
        "clock_switch_grid",
        metal_layer="global_clk_switch_network",
        startx="0",
        repeatx=f"W/{denom}",
        starty="0",
        repeaty=f"H/{denom}",
        chan_w="4",
        switch_name="drive_buff",
        switch_block_type="subset",
    )

    grid.append(comment("Put a drive in the middle of the device"))
    etree.SubElement(
        grid,
        "switch_point",
        type="drive",
        name="drive",
        xoffset=frac(n, "W", denom),
        yoffset=frac(n, "H", denom),
        switch_name="drive_buff",
    )

    grid.append(comment("Tap into each quadrant's spine."))
    for row in range(1, n + 1):
        for col in range(1, n + 1):
            etree.SubElement(
                grid,
                "switch_point",
                type="tap",
                name=f"tap_q{qname(col, row)}",
                xoffset=frac(2 * col - 1, "W", denom),
                yoffset=frac(2 * row - 1, "H", denom),
            )

    return network


def build_spine(col: int, row: int, n: int, denom: int) -> Any:
    """Build one quadrant's spine clock_network element.

    The spine spans the full device height belonging to its row band (one
    Nth of the device height) at the x-center of its column, driven at the
    band's vertical midpoint.
    """
    starty = frac(2 * row - 2, "H", denom)
    endy = "H" if row == n else f"{frac(2 * row, 'H', denom)} - 1"
    x = frac(2 * col - 1, "W", denom)

    network = etree.Element("clock_network", name=f"spine_q{qname(col, row)}", num_inst="2")
    spine = etree.SubElement(network, "spine", metal_layer="global_spine", x=x, starty=starty, endy=endy)
    etree.SubElement(spine, "switch_point", type="drive", name="drive", yoffset=frac(1, "H", denom), switch_name="drive_buff")
    etree.SubElement(spine, "switch_point", type="tap", name="tap", yoffset="0", yincr="1")
    return network


def build_rib(col: int, row: int, n: int, denom: int) -> Any:
    """Build one quadrant's rib clock_network element.

    The rib spans the full device width belonging to its column band (one
    Nth of the device width) at the y-center of its row, driven at the
    band's horizontal midpoint.
    """
    starty = frac(2 * row - 2, "H", denom)
    endy = "H" if row == n else frac(2 * row, "H", denom)
    startx = frac(2 * col - 2, "W", denom)
    endx = "W" if col == n else f"{frac(2 * col, 'W', denom)} - 1"

    network = etree.Element("clock_network", name=f"ribs_q{qname(col, row)}", num_inst="2")
    rib = etree.SubElement(network, "rib", metal_layer="global_rib", y=starty, endy=endy, startx=startx, endx=endx, repeaty="1")
    etree.SubElement(rib, "switch_point", type="drive", name="drive", xoffset=frac(1, "W", denom), switch_name="drive_buff")
    etree.SubElement(rib, "switch_point", type="tap", name="tap", xoffset="0", xincr="1")
    return network


def build_clock_routing(n: int, denom: int) -> Any:
    """Build the clock_routing element wiring the driver through to CLOCK pins."""
    routing = etree.Element("clock_routing")

    routing.append(comment("Connect the driver to the center of the grid."))
    etree.SubElement(
        routing,
        "tap",
        **{"from": "ROUTING", "to": "global_clk_switch_network.drive"},
        locationx=frac(n, "W", denom),
        locationy=frac(n, "H", denom),
        switch="0",
        fc_val="1.0",
    )

    routing.append(comment("Connect the clock switch grid to the quadrant spines."))
    for row in range(1, n + 1):
        for col in range(1, n + 1):
            etree.SubElement(
                routing,
                "tap",
                **{"from": f"global_clk_switch_network.tap_q{qname(col, row)}", "to": f"spine_q{qname(col, row)}.drive"},
                switch="0",
                fc_val="1.0",
            )

    routing.append(comment("Connect each spine to its own quadrant's rib."))
    for row in range(1, n + 1):
        for col in range(1, n + 1):
            etree.SubElement(
                routing,
                "tap",
                **{"from": f"spine_q{qname(col, row)}.tap", "to": f"ribs_q{qname(col, row)}.drive"},
                switch="0",
                fc_val="1.0",
            )

    routing.append(comment("Connect the ribs to the clock pins."))
    for row in range(1, n + 1):
        for col in range(1, n + 1):
            etree.SubElement(
                routing,
                "tap",
                **{"from": f"ribs_q{qname(col, row)}.tap", "to": "CLOCK"},
                switch="ipin_cblock",
                fc_val="1",
            )

    return routing


def build_clocknetworks(n: int) -> Any:
    """Build the full <clocknetworks> element for an NxN disjoint-quadrant grid.

    Args:
        n: Grid size. n=2 reproduces the hand-written 2x2 architecture.

    Returns:
        The <clocknetworks> element, ready to replace the template's.
    """
    denom = 2 * n

    clocknetworks = etree.Element("clocknetworks")

    metal_layers = etree.SubElement(clocknetworks, "metal_layers")
    for name in ("global_clk_switch_network", "global_spine", "global_rib"):
        etree.SubElement(metal_layers, "metal_layer", name=name, Rmetal="50.42", Cmetal="20.7e-15")

    clocknetworks.append(comment("Create a global clk switch network of sparse switches."))
    clocknetworks.append(build_switch_grid(n, denom))

    for row in range(1, n + 1):
        for col in range(1, n + 1):
            clocknetworks.append(build_spine(col, row, n, denom))

    clocknetworks.append(comment("Create the ribs for each quadrant, driven from their centers."))
    for row in range(1, n + 1):
        for col in range(1, n + 1):
            clocknetworks.append(build_rib(col, row, n, denom))

    clocknetworks.append(build_clock_routing(n, denom))

    return clocknetworks


def generate_arch(n: int, template_path: str, output_dir: str, parser: Any) -> None:
    """Generate one NxN architecture variant from the base template.

    Args:
        n: Grid size to generate.
        template_path: Path to the base 2x2 architecture XML template.
        output_dir: Directory to write the generated architecture into.
        parser: lxml.etree parser used to read the template while preserving
            comments.

    Returns:
        None. The generated file is written to output_dir.
    """
    xml_tree = etree.parse(template_path, parser)
    xml_root = xml_tree.getroot()

    old_clocknetworks = xml_root.find("clocknetworks")
    if old_clocknetworks is None:
        raise ValueError(f"'{template_path}' has no <clocknetworks> element to replace.")

    new_clocknetworks = build_clocknetworks(n)
    # Match the template's 2-space-per-level indentation. level=1 since
    # <clocknetworks> is a direct child of the <architecture> root.
    etree.indent(new_clocknetworks, space="  ", level=1)
    new_clocknetworks.tail = old_clocknetworks.tail

    old_clocknetworks.addprevious(new_clocknetworks)
    xml_root.remove(old_clocknetworks)

    output_path = os.path.join(output_dir, OUTPUT_NAME_FMT.format(n=n))
    xml_tree.write(output_path, encoding="UTF-8", xml_declaration=False, pretty_print=True)
    print(f"Generated: {output_path}")


def generate_spine_region_archs(sizes: "list[int]", template_path: str, output_dir: str = ".") -> None:
    """Generate NxN disjoint-quadrant clock network architecture variants.

    Args:
        sizes: Grid sizes to generate, e.g. [16, 32].
        template_path: Path to the base 2x2 architecture XML template.
        output_dir: Directory to write generated architecture files into.

    Returns:
        None. Generated architecture files are written to output_dir.
    """
    if not os.path.exists(template_path):
        print(f"Error: Template {template_path} not found.")
        return

    # Using lxml parser to retain existing XML comments during tree transformation
    parser = etree.XMLParser(remove_comments=False)

    for n in sizes:
        generate_arch(n, template_path, output_dir, parser)


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    generate_spine_region_archs([4, 8, 16, 32], os.path.join(script_dir, TEMPLATE_NAME), script_dir)
