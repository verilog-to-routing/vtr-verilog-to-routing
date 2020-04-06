.. _heterogeneous_tiles_tutorial:

Heterogeneous tiles tutorial
============================

This tutorial aims at providing information to the user on how to model sub tiles to enable ``heterogeneous tiles`` in VPR.

To correctly model an architecture, ``each physical tile`` requires at least one sub tile definition. This represents a default
homogeneous architecture, composed of one or many capacity instances of the sub tile within the physical tile.

To enhance the expressivity of VPR architecture, additional sub tiles can be inserted alongside with the default sub tile.
This enables the definition of so called ``heterogeneous tiles``.

An ``heterogeneous tile`` is a tile that includes a two or more site types that may differ in the following aspects:

- ``Fc definition``
- ``pinlocations``
- ``IO ports definition``

With this new capability, the device grid of a given architecture does include a new ``depth`` coordinate that identifies
the type of sub tile used and its actual location, in case the capacity is greater than 1.

A visual representation of the new VPR grid is the following:

.. image:: sub_tiles_grid.png

Heterogeneous tiles examples
----------------------------

1. Different pin locations among sub tiles.

   The Xilinx Series 7 Clock tile is composed of 16 BUFGCTRL sites. Even though they are equivalent regarding the ports and Fc
   definition, some of the sites differ in terms of pin locations.

   Heterogeneous tiles come in hand to model this kind of tiles and an example is the following:

.. code-block:: xml

    <tiles>
        <tile name="BUFG_TILE">
            <sub_tile name="BUFGCTRL_0" capacity="4">
                <clock name="I0" num_pins="1"/>
                <clock name="I1" num_pins="1"/>
                <input name="CE0" num_pins="1"/>
                <input name="CE1" num_pins="1"/>
                <input name="IGNORE0" num_pins="1"/>
                <input name="IGNORE1" num_pins="1"/>
                <input name="S0" num_pins="1"/>
                <input name="S1" num_pins="1"/>
                <output name="O" num_pins="1"/>
                <fc in_type="abs" in_val="2" out_type="abs" out_val="2"/>
                <pinlocations pattern="custom">
                    <loc side="top">BUFGCTRL_0.I1 BUFGCTRL_0.I0 BUFGCTRL_0.CE0 BUFGCTRL_0.S0 BUFGCTRL_0.IGNORE1 BUFGCTRL_0.CE1 BUFGCTRL_0.IGNORE0 BUFGCTRL_0.S1</loc>
                    <loc side="right">BUFGCTRL_0.I1 BUFGCTRL_0.I0 BUFGCTRL_0.O</loc>
                </pinlocations>
                <equivalent_sites>
                  <site pb_type="BUFGCTRL" pin_mapping="direct"/>
                </equivalent_sites>
            </sub_tile>
            <sub_tile name="BUFGCTRL_1" capacity="8">
                <clock name="I0" num_pins="1"/>
                <clock name="I1" num_pins="1"/>
                <input name="CE0" num_pins="1"/>
                <input name="CE1" num_pins="1"/>
                <input name="IGNORE0" num_pins="1"/>
                <input name="IGNORE1" num_pins="1"/>
                <input name="S0" num_pins="1"/>
                <input name="S1" num_pins="1"/>
                <output name="O" num_pins="1"/>
                <fc in_type="abs" in_val="2" out_type="abs" out_val="2"/>
                <pinlocations pattern="custom">
                    <loc side="top">BUFGCTRL_1.S1 BUFGCTRL_1.CLK_BUFG_I0 BUFGCTRL_1.CE1 BUFGCTRL_1.CLK_BUFG_I1 BUFGCTRL_1.IGNORE1 BUFGCTRL_1.IGNORE0 BUFGCTRL_1.CE0 BUFGCTRL_1.S0</loc>
                    <loc side="right">BUFGCTRL_1.CLK_BUFG_I0 BUFGCTRL_1.CLK_BUFG_I1 BUFGCTRL_1.CLK_BUFG_O</loc>
                </pinlocations>
                <equivalent_sites>
                  <site pb_type="BUFGCTRL" pin_mapping="direct"/>
                </equivalent_sites>
            </sub_tile>
            <sub_tile name="BUFGCTRL_2" capacity="4">
                <clock name="I0" num_pins="1"/>
                <clock name="I1" num_pins="1"/>
                <input name="CE0" num_pins="1"/>
                <input name="CE1" num_pins="1"/>
                <input name="IGNORE0" num_pins="1"/>
                <input name="IGNORE1" num_pins="1"/>
                <input name="S0" num_pins="1"/>
                <input name="S1" num_pins="1"/>
                <output name="O" num_pins="1"/>
                <fc in_type="abs" in_val="2" out_type="abs" out_val="2"/>
                <pinlocations pattern="custom">
                    <loc side="right">BUFGCTRL_1.S1 BUFGCTRL_1.CLK_BUFG_I0 BUFGCTRL_1.CE1 BUFGCTRL_1.CLK_BUFG_I1 BUFGCTRL_1.IGNORE1 BUFGCTRL_1.IGNORE0 BUFGCTRL_1.CE0 BUFGCTRL_1.S0</loc>
                    <loc side="left">BUFGCTRL_1.CLK_BUFG_I0 BUFGCTRL_1.CLK_BUFG_I1 BUFGCTRL_1.CLK_BUFG_O</loc>
                </pinlocations>
                <equivalent_sites>
                  <site pb_type="BUFGCTRL" pin_mapping="direct"/>
                </equivalent_sites>
            </sub_tile>
        </tile>
    </tiles>

    <complexblocklist>
        <pb_type name="BUFGCTRL"/>
            <clock name="I0" num_pins="1"/>
            <clock name="I1" num_pins="1"/>
            <input name="CE0" num_pins="1"/>
            <input name="CE1" num_pins="1"/>
            <input name="IGNORE0" num_pins="1"/>
            <input name="IGNORE1" num_pins="1"/>
            <input name="S0" num_pins="1"/>
            <input name="S1" num_pins="1"/>
            <output name="O" num_pins="1"/>
            <mode ...>
            ...
        </pb_type>
    </complexblocklist>

2. Different sub tile types.

   As another example taken from the Xilinx Series 7 fabric, the HCLK_IOI tile is composed of three different site types, namely BUFIO, BUFR and IDELAYCTRL.

   Each one of these site types has different IO pins as well as pin locations.

   A visual representation of this case is the following:

.. image:: hclk_ioi.png

.. code-block:: xml

    <tile name="HCLK_IOI">
        <sub_tile name="BUFIO" capacity="4">
            <clock name="I" num_pins="1"/>
            <output name="O" num_pins = "1"/>
            <equivalent_sites>
                <site name="BUFIO_SITE" pin_mapping="direct"/>
            </equivalent_sites>
            <fc ...>
            <pinlocations ...>
        </sub_tile>
        <sub_tile name="BUFR" capacity="4">
            <clock name="I" num_pins="1"/>
            <input name="CE" num_pins="1"/>
            <output name="O" num_pins = "1"/>
            <equivalent_sites>
                <site name="BUFR_SITE" pin_mapping="direct"/>
            </equivalent_sites>
            <fc ...>
            <pinlocations ...>
        </sub_tile>
        <sub_tile name="IDELAYCTRL" capacity="1">
            <clock name="REFCLK" num_pins="1"/>
            <output name="RDY" num_pins="1"/>
            <equivalent_sites>
                <site name="IDELAYCTRL_SITE" pin_mapping="direct"/>
            </equivalent_sites>
            <fc ...>
            <pinlocations ...>
        </sub_tile>
    </tile>
