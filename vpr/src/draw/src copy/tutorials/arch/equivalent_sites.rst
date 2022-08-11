.. _equivalent_sites_tutorial:

Equivalent Sites tutorial
=========================

This tutorial aims at providing information to the user on how to model the equivalent sites to enable ``equivalent placement`` in VPR.

Equivalent site placement allows the user to define complex logical blocks (top-level pb_types) that can be used in multiple physical location types of the FPGA device grid.
In the same way, the user can define many physical tiles that have different physical attributes that can implement the same logical block.

The first case (multiple physical grid location types for one complex logical block) is explained below.
The device has at disposal two different Configurable Logic Blocks (CLB), SLICEL and SLICEM.
In this case, the SLICEM CLB is a superset that implements additional features w.r.t. the SLICEL CLB.
Therefore, the user can decide to model the architecture to be able to place the SLICEL Complex Block in a SLICEM physical tile, being it a valid grid location.
This behavior can lead to the generation of more accurate and better placement results, given that a Complex Logic Block is not bound to only one physical location type.

Below the user can find the implementation of this situation starting from an example that does not make use of the equivalent site placement:

.. code-block:: xml

    <tiles>
        <tile name="SLICEL_TILE">
            <input name="IN_A" num_pins="6"/>
            <input name="AX" num_pins="1"/>
            <input name="SR" num_pins="1"/>
            <input name="CE" num_pins="1"/>
            <input name="CIN" num_pins="1"/>
            <clock name="CLK" num_pins="1"/>
            <output name="A" num_pins="1"/>
            <output name="AMUX" num_pins="1"/>
            <output name="AQ" num_pins="1"/>

            <equivalent_sites>
                <site pb_type="SLICEL_SITE" pin_mapping="direct"/>
            </equivalent_sites>

            <fc />
            <pinlocations />
        </tile>
        <tile name="SLICEM_TILE">
            <input name="IN_A" num_pins="6"/>
            <input name="AX" num_pins="1"/>
            <input name="AI" num_pins="1"/>
            <input name="SR" num_pins="1"/>
            <input name="WE" num_pins="1"/>
            <input name="CE" num_pins="1"/>
            <input name="CIN" num_pins="1"/>
            <clock name="CLK" num_pins="1"/>
            <output name="A" num_pins="1"/>
            <output name="AMUX" num_pins="1"/>
            <output name="AQ" num_pins="1"/>

            <equivalent_sites>
                <site pb_type="SLICEM_SITE" pin_mapping="direct"/>
            </equivalent_sites>

            <fc />
            <pinlocations />
        </tile>
    </tiles>

    <complexblocklist>
        <pb_type name="SLICEL_SITE"/>
            <input name="IN_A" num_pins="6"/>
            <input name="AX" num_pins="1"/>
            <input name="AI" num_pins="1"/>
            <input name="SR" num_pins="1"/>
            <input name="CE" num_pins="1"/>
            <input name="CIN" num_pins="1"/>
            <clock name="CLK" num_pins="1"/>
            <output name="A" num_pins="1"/>
            <output name="AMUX" num_pins="1"/>
            <output name="AQ" num_pins="1"/>
            <mode />
            /
        </pb_type>
        <pb_type name="SLICEM_SITE"/>
            <input name="IN_A" num_pins="6"/>
            <input name="AX" num_pins="1"/>
            <input name="SR" num_pins="1"/>
            <input name="WE" num_pins="1"/>
            <input name="CE" num_pins="1"/>
            <input name="CIN" num_pins="1"/>
            <clock name="CLK" num_pins="1"/>
            <output name="A" num_pins="1"/>
            <output name="AMUX" num_pins="1"/>
            <output name="AQ" num_pins="1"/>
            <mode />
            /
        </pb_type>
    </complexblocklist>

As the user can see, ``SLICEL`` and ``SLICEM`` are treated as two different entities, even though they seem to be similar one to another.
To have the possibility to make VPR choose a ``SLICEM`` location when placing a ``SLICEL_SITE`` pb_type, the user needs to change the ``SLICEM`` tile accordingly, as shown below:

.. code-block:: xml

    <tile name="SLICEM_TILE">
        <input name="IN_A" num_pins="6"/>
        <input name="AX" num_pins="1"/>
        <input name="AI" num_pins="1"/>
        <input name="SR" num_pins="1"/>
        <input name="WE" num_pins="1"/>
        <input name="CE" num_pins="1"/>
        <input name="CIN" num_pins="1"/>
        <clock name="CLK" num_pins="1"/>
        <output name="A" num_pins="1"/>
        <output name="AMUX" num_pins="1"/>
        <output name="AQ" num_pins="1"/>

        <equivalent_sites>
            <site pb_type="SLICEM_SITE" pin_mapping="direct"/>
            <site pb_type="SLICEL_SITE" pin_mapping="custom">
                <direct from="SLICEM_TILE.IN_A" to="SLICEL_SITE.IN_A"/>
                <direct from="SLICEM_TILE.AX" to="SLICEL_SITE.AX"/>
                <direct from="SLICEM_TILE.SR" to="SLICEL_SITE.SR"/>
                <direct from="SLICEM_TILE.CE" to="SLICEL_SITE.CE"/>
                <direct from="SLICEM_TILE.CIN" to="SLICEL_SITE.CIN"/>
                <direct from="SLICEM_TILE.CLK" to="SLICEL_SITE.CLK"/>
                <direct from="SLICEM_TILE.A" to="SLICEL_SITE.A"/>
                <direct from="SLICEM_TILE.AMUX" to="SLICEL_SITE.AMUX"/>
                <direct from="SLICEM_TILE.AQ" to="SLICEL_SITE.AQ"/>
            </site>
        </equivalent_sites>

        <fc />
        <pinlocations />
    </tile>

With the above description of the ``SLICEM`` tile, the user can now have the ``SLICEL`` sites to be placed in ``SLICEM`` physical locations.
One thing to notice is that not all the pins have been mapped for the ``SLICEL_SITE``. For instance, the ``WE`` and ``AI`` port are absent from the ``SLICEL_SITE`` definition, hence they cannot appear in the pin mapping between physical tile and logical block.

The second case described in this tutorial refers to the situation for which there are multiple different physical location types in the device grid that are used by one complex logical blocks.
Imagine the situation for which the device has left and right I/O tile types which have different pinlocations, hence they need to be defined in two different ways.
With equivalent site placement, the user doesn't need to define multiple different pb_types that implement the same functionality.

Below the user can find the implementation of this situation starting from an example that does not make use of the equivalent site placement:

.. code-block:: xml

    <tiles>
        <tile name="LEFT_IOPAD_TILE">
            <input name="INPUT" num_pins="1"/>
            <output name="OUTPUT" num_pins="1"/>

            <equivalent_sites>
                <site pb_type="LEFT_IOPAD_SITE" pin_mapping="direct"/>
            </equivalent_sites>

            <fc />
            <pinlocations pattern="custom">
                <loc side="left">LEFT_IOPAD_TILE.INPUT</loc>
                <loc side="right">LEFT_IOPAD_TILE.OUTPUT</loc>
            </pinlocations>
        </tile>
        <tile name="RIGHT_IOPAD_TILE">
            <input name="INPUT" num_pins="1"/>
            <output name="OUTPUT" num_pins="1"/>

            <equivalent_sites>
                <site pb_type="RIGHT_IOPAD_SITE" pin_mapping="direct"/>
            </equivalent_sites>

            <fc />
            <pinlocations pattern="custom">
                <loc side="right">RIGHT_IOPAD_TILE.INPUT</loc>
                <loc side="left">RIGHT_IOPAD_TILE.OUTPUT</loc>
            </pinlocations>
        </tile>
    </tiles>

    <complexblocklist>
        <pb_type name="LEFT_IOPAD_SITE">
            <input name="INPUT" num_pins="1"/>
            <output name="OUTPUT" num_pins="1"/>
            <mode />
            /
        </pb_type>
        <pb_type name="RIGHT_IOPAD_SITE">
            <input name="INPUT" num_pins="1"/>
            <output name="OUTPUT" num_pins="1"/>
            <mode />
            /
        </pb_type>
    </complexblocklist>

To avoid duplicating the complex logic blocks in ``LEFT`` and ``RIGHT IOPADS``, the user can describe the pb_type only once and add it to the equivalent sites tag of the two different tiles, as follows:

.. code-block:: xml

    <tiles>
        <tile name="LEFT_IOPAD_TILE">
            <input name="INPUT" num_pins="1"/>
            <output name="OUTPUT" num_pins="1"/>

            <equivalent_sites>
                <site pb_type="IOPAD_SITE" pin_mapping="direct"/>
            </equivalent_sites>

            <fc />
            <pinlocations pattern="custom">
                <loc side="left">LEFT_IOPAD_TILE.INPUT</loc>
                <loc side="right">LEFT_IOPAD_TILE.OUTPUT</loc>
            </pinlocations>
        </tile>
        <tile name="RIGHT_IOPAD_TILE">
            <input name="INPUT" num_pins="1"/>
            <output name="OUTPUT" num_pins="1"/>

            <equivalent_sites>
                <site pb_type="IOPAD_SITE" pin_mapping="direct"/>
            </equivalent_sites>

            <fc />
            <pinlocations pattern="custom">
                <loc side="right">RIGHT_IOPAD_TILE.INPUT</loc>
                <loc side="left">RIGHT_IOPAD_TILE.OUTPUT</loc>
            </pinlocations>
        </tile>
    </tiles>

    <complexblocklist>
        <pb_type name="IOPAD_SITE">
            <input name="INPUT" num_pins="1"/>
            <output name="OUTPUT" num_pins="1"/>
            <mode>
                ...
            </mode>
        </pb_type>
    </complexblocklist>

With this implementation, the ``IOPAD_SITE`` can be placed both in the ``LEFT`` and ``RIGHT`` physical location types.
Note that the pin_mapping is set as ``direct``, given that the physical tile and the logical block share the same IO pins.

The two different cases can be mixed to have a N to M mapping of physical tiles/logical blocks.
