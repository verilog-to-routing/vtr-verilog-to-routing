#!/usr/bin/env python

"""
This script is intended to modify the architecture description file to be compliant with
the new format.

It moves the top level pb_types attributes and tags to the tiles high-level tag.

BEFORE:
<complexblocklist>
    <pb_type name="BRAM" area="2" height="4" width="1" capacity="1">
        <input ... />
        <input ... />
        <input ... />
        <output ... />
        <output ... />
        <output ... />
        <interconnect ... />
        <fc ... />
        <pinlocations ... />
        <switchblock_locations ... />
    </pb_type>
</complexblocklist>

AFTER:
<tiles>
    <tile name="BRAM" area="2" height="4" width="1" capacity="1">
        <interconnect ... />
        <fc ... />
        <pinlocations ... />
        <switchblock_locations ... />
    </tile>
</tiles>
<complexblocklist
    <pb_type name="BRAM">
        <input ... />
        <input ... />
        <input ... />
        <output ... />
        <output ... />
        <output ... />
    </pb_type>
</complexblocklist>
"""

"""
This script is intended to modify the architecture description file to be compliant with
the new format.

It moves the top level pb_types attributes and tags to the tiles high-level tag.

BEFORE:
<complexblocklist>
    <pb_type name="BRAM" area="2" height="4" width="1" capacity="1">
        <input ... />
        <input ... />
        <input ... />
        <output ... />
        <output ... />
        <output ... />
        <interconnect ... />
        <fc ... />
        <pinlocations ... />
        <switchblock_locations ... />
    </pb_type>
</complexblocklist>

AFTER:
<tiles>
    <tile name="BRAM" area="2" height="4" width="1" capacity="1">
        <interconnect ... />
        <fc ... />
        <pinlocations ... />
        <switchblock_locations ... />
    </tile>
</tiles>
<complexblocklist
    <pb_type name="BRAM">
        <input ... />
        <input ... />
        <input ... />
        <output ... />
        <output ... />
        <output ... />
    </pb_type>
</complexblocklist>
"""

from lxml import etree as ET
import argparse

TAGS_TO_SWAP = ['fc', 'pinlocations', 'switchblock_locations']
ATTR_TO_REMOVE = ['area', 'height', 'width', 'capacity']

def swap_tags(tile, pb_type):
    # Moving tags from top level pb_type to tile
    for child in pb_type:
        if child.tag in TAGS_TO_SWAP:
            pb_type.remove(child)
            tile.append(child)


def main():
    parser = argparse.ArgumentParser(
        description="Moves top level pb_types to tiles tag."
    )
    parser.add_argument(
        '--arch_xml',
        required=True,
        help="Input arch.xml that needs to be modified to move the top level pb_types to the `tiles` tag."
    )

    args = parser.parse_args()

    arch_xml = ET.ElementTree()
    root_element = arch_xml.parse(args.arch_xml)

    tiles = ET.SubElement(root_element, 'tiles')

    top_pb_types = []
    for pb_type in root_element.iter('pb_type'):
        if pb_type.getparent().tag == 'complexblocklist':
            top_pb_types.append(pb_type)

    for pb_type in top_pb_types:
        tile = ET.SubElement(tiles, 'tile')
        attrs = pb_type.attrib

        for attr in attrs:
            tile.set(attr, pb_type.get(attr))

        # Remove attributes of top level pb_types only
        for attr in ATTR_TO_REMOVE:
            pb_type.attrib.pop(attr, None)

        swap_tags(tile, pb_type)

    print(ET.tostring(arch_xml, pretty_print=True).decode('utf-8'))


if __name__ == '__main__':
    main()
