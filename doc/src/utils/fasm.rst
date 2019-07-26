.. _genfasm:

FPGA Assembly (FASM) Output Support
===================================

After VPR has generated a placed and routed design, the ``genfasm`` utility can
emit a FASM_ file to represent the design at a level detailed enough to allow generation of a bitstream to program a device. This FASM output file is enabled by FASM metadata encoded in the VPR
architecture definition and routing graph.  The output FASM file can be
converted into a bitstream format suitable to program the target architecture via architecture specific tooling. Current devices that can be programmed using the vpr + fasm flow include Lattice iCE40 and Xilinx Artix-7 devices, with work on more devices underway. More information on supported devices is available from the Symbiflow_ website.

.. _FASM: https://github.com/SymbiFlow/fasm
.. _Symbiflow: https://symbiflow.github.io

FASM metadata
-------------

The ``genfasm`` utility uses ``metadata`` blocks (see :ref:`arch_metadata`)
attached to the architecture definition and routing graph to emit FASM
features.  By adding FASM specific metadata to both the architecture
definition and the routing graph, a FASM file that represents the place and
routed design can be generated.

All metadata tags are ignored when packing, placing and routing.  After VPR has
been completed placement, ``genfasm`` utility loads the VPR output files
(.net, .place, .route) and then uses the FASM metadata to emit a FASM file.
The following metadata "keys" are recognized by ``genfasm``:

 * "fasm_prefix"
 * "fasm_features"
 * "fasm_type" and "fasm_lut"
 * "fasm_mux"
 * "fasm_params"

Invoking genfasm
----------------

``genfasm`` expects that place and route on the design is completed (e.g.
.net, .place, .route files are present), so ensure that routing is complete
before executing ``genfasm``.  ``genfasm`` should be invoked in the same
subdirectory as the routing output.  The output FASM file will be written to
``<blif root>.fasm``.

FASM prefixing
--------------

FASM feature names has structure through their prefixes.  In general the first
part of the FASM feature is the location of the feature, such as the name of
the tile the feature is located in, e.g. INT_L_X5Y6 or CLBLL_L_X10Y12.  The
next part is typically an identifier within the tile.  For example a CLBLL
tile has two slices, so the next part of the FASM feature name is the slice
identifier, e.g. SLICE_X0 or SLICE_X1.

Now consider the CLBLL_L pb_type.  This pb_type is repeated in the grid for
each tile of that type.  To allow one pb_type definition to be defined, the
"fasm_prefix" metadata tag is allowed to be attached at the layout level on
the <single> tag.  This enables the same pb_type to be used for all CLBLL_L
tiles, and the "fasm_prefix" is prepended to all FASM metadata within that
pb_type.  For example:

.. code-block:: xml

      <single priority="1" type="BLK_TI-CLBLL_L" x="35" y="51">
        <metadata>
          <meta name="fasm_prefix">CLBLL_L_X12Y100</meta>
        </metadata>
      </single>
      <single priority="1" type="BLK_TI-CLBLL_L" x="35" y="50">
        <metadata>
          <meta name="fasm_prefix">CLBLL_L_X12Y101</meta>
        </metadata>
      </single>

"fasm_prefix" tags can also be used within a pb_type to handle repeated
features.  For example in the CLB, there are 4 LUTs that can be described by
a common pb_type, except that the prefix changes for each.  For example,
consider the FF's within a CLB.  There are 8 FF's that share a common
structure, except for a prefix change.  "fasm_prefix" can be a space
separated list to assign prefixes to the index of the pb_type, rather than
needing to emit N copies of the pb_type with varying prefixes.

.. code-block:: xml

    <pb_type name="BEL_FF-FDSE_or_FDRE" num_pb="8">
      <input  name="D" num_pins="1"/>
      <input  name="CE" num_pins="1"/>
      <clock  name="C" num_pins="1"/>
      <input  name="SR" num_pins="1"/>
      <output name="Q" num_pins="1"/>
      <metadata>
        <meta name="fasm_prefix">AFF BFF CFF DFF A5FF B5FF C5FF D5FF</meta>
      </metadata>
    </pb_type>

Construction of the prefix
~~~~~~~~~~~~~~~~~~~~~~~~~~

"fasm_prefix" is accumulated throughout the structure of the architecture
definition.  Each "fasm_prefix" is joined together with a period ('.'), and
then a period is added after the prefix before the FASM feature name.


Simple FASM feature emissions
-----------------------------

In cases where a FASM feature needs to be emitted simply via use of a pb_type,
the "fasm_features" tag can be used.  If the pb_type (or mode) is selected,
then all "fasm_features" in the metadata will be emitted.  Multiple features
can be listed, whitespace separated.  Example:

.. code-block:: xml

    <metadata>
        <meta name="fasm_features">ZRST</meta>
    </metadata>

The other place that "fasm_features" is used heavily is on <edge> tags in the
routing graph.  If an edge is used in the final routed design, "genfasm" will
emit features attached to the edge.  Example:

.. code-block:: xml

    <edge sink_node="431195" src_node="418849" switch_id="0">
      <metadata>
        <meta name="fasm_features">HCLK_R_X58Y130.HCLK_LEAF_CLK_B_TOP4.HCLK_CK_BUFHCLK7 HCLK_R_X58Y130.ENABLE_BUFFER.HCLK_CK_BUFHCLK7</meta>
      </metadata>
    </edge>

In this example, when the routing graph connects node 418849 to 431195, two
FASM features will be emitted:

 * ``HCLK_R_X58Y130.HCLK_LEAF_CLK_B_TOP4.HCLK_CK_BUFHCLK7``
 * ``HCLK_R_X58Y130.ENABLE_BUFFER.HCLK_CK_BUFHCLK7``

Emitting LUTs
-------------

LUTs are a structure that is explicitly understood by VPR.  In order to emit
LUTs, two metadata keys must be used, "fasm_type" and "fasm_lut".  "fasm_type"
must be either "LUT" or "SPLIT_LUT".  The "fasm_type" modifies how the
"fasm_lut" key is interpreted.  If the pb_type that the metadata is attached
to has no "num_pb" or "num_pb" equals 1, then "fasm_type" can be "LUT".
"fasm_lut" is then the feature that represents the LUT table storage features,
example:

.. code-block:: xml

   <metadata>
     <meta name="fasm_type">LUT</meta>
     <meta name="fasm_lut">
       ALUT.INIT[63:0]
     </meta>
   </metadata>

FASM LUT metadata must be attached to the ``<pb_type>`` at or within the
``<mode>`` tag directly above the ``<pb_type>`` with ``blif_model=".names"``.
Do note that there is an implicit ``<mode>`` tag within intermediate
``<pb_type>`` when no explicit ``<mode>`` tag is present. The FASM LUT
metadata tags will not be recognized attached inside of ``<pb_type>``'s higher
above the leaf type.

When specifying a FASM features with more than one bit, explicitly specify the
bit range being set.  This is required because "genfasm" does not have access
to the actual bit database, and would otherwise not have the width of the
feature.

When "fasm_type" is "SPLIT_LUT", "fasm_lut" must specify both the feature that
represents the LUT table storage features and the pb_type path to the LUT
being specified.  Example:

.. code-block:: xml

   <metadata>
     <meta name="fasm_type">SPLIT_LUT</meta>
     <meta name="fasm_lut">
       ALUT.INIT[31:0] = BEL_LT-A5LUT[0]
       ALUT.INIT[63:32] = BEL_LT-A5LUT[1]
     </meta>
   </metadata>

In this case, the LUT in pb_type BEL_LT-A5LUT[0] will use INIT[31:0], and the
LUT in pb_type BEL_LT-A5LUT[1] will use INIT[63:32].

Within tile interconnect features
---------------------------------

When a tile has interconnect feature, e.g. output muxes, the "fasm_mux" tag
should be attached to the interconnect tag, likely the ``<direct>`` or
``<mux>`` tags.  From the perspective of genfasm, the ``<direct>`` and
``<mux>`` tags are equivalent.  The syntax for the "fasm_mux" newline
separated relationship between mux input wire names and FASM features.
Example:

.. code-block:: xml

    <mux name="D5FFMUX" input="BLK_IG-COMMON_SLICE.DX BLK_IG-COMMON_SLICE.DO5" output="BLK_BB-SLICE_FF.D5[3]" >
      <metadata>
        <meta name="fasm_mux">
          BLK_IG-COMMON_SLICE.DO5 : D5FFMUX.IN_A
          BLK_IG-COMMON_SLICE.DX : D5FFMUX.IN_B
        </meta>
      </metadata>
    </mux>

The above mux connects input BLK_IG-COMMON_SLICE.DX or BLK_IG-COMMON_SLICE.DO5
to BLK_BB-SLICE_FF.D5[3].  When VPR selects BLK_IG-COMMON_SLICE.DO5 for the
mux, "genfasm" will emit D5FFMUX.IN_A, etc.

There is not a requirement that all inputs result in a feature being set.
In cases where some mux selections result in no feature being set, use "NULL"
as the feature name.  Example:

.. code-block:: xml

    <mux name="CARRY_DI3" input="BLK_IG-COMMON_SLICE.DO5 BLK_IG-COMMON_SLICE.DX" output="BEL_BB-CARRY[2].DI" >
      <metadata>
        <meta name="fasm_mux">
          BLK_IG-COMMON_SLICE.DO5 : CARRY4.DCY0
          BLK_IG-COMMON_SLICE.DX : NULL
        </meta>
      </metadata>
    </mux>

The above examples all used the ``<mux>`` tag.  The "fasm_mux" metadata key
can also be used with the ``<direct>`` tag in the same way, example:

.. code-block:: xml

    <direct name="WA7"  input="BLK_IG-SLICEM.CX" output="BLK_IG-SLICEM_MODES.WA7">
      <metadata>
        <meta name="fasm_mux">
          BLK_IG-SLICEM.CX = WA7USED
        </meta>
      </metadata>
    </direct>

If multiple FASM features are required for a mux, they can be specified using
comma's as a seperator.  Example:

.. code-block:: xml

    <mux name="D5FFMUX" input="BLK_IG-COMMON_SLICE.DX BLK_IG-COMMON_SLICE.DO5" output="BLK_BB-SLICE_FF.D5[3]" >
      <metadata>
        <meta name="fasm_mux">
          BLK_IG-COMMON_SLICE.DO5 : D5FFMUX.IN_A
          BLK_IG-COMMON_SLICE.DX : D5FFMUX.IN_B, D5FF.OTHER_FEATURE
        </meta>
      </metadata>
    </mux>

Passing parameters through to the FASM Output
---------------------------------------------

In many cases there are parameters that need to be passed directly from the
input :ref:`vpr_eblif_file` to the FASM file.  These can be passed into a FASM
feature via the "fasm_params" key.  Note that care must be taken to have the
"fasm_params" metadata be attached to pb_type that the packer uses, the
pb_type with the blif_model= ".subckt".

The "fasm_params" value is a newline separated list of FASM features to eblif
parameters. Example:

.. code-block:: xml

  <metadata>
    <meta name="fasm_params">
      INIT[31:0] = INIT_00
      INIT[63:32] = INIT_01
    </meta>
  </metadata>

The FASM feature is on the left hand side of the equals.  When setting a
parameter with multiple bits, the bit range must be specified.  If the
parameter is a single bit, the bit range is not required, but can be supplied
for clarity.  The right hand side is the parameter name from eblif.  If the
parameter name is not found in the eblif, that FASM feature will not be
emitted.

No errors or warnings will be generated for unused parameters from eblif or
unused mappings between eblif parameters and FASM parameters to allow for
flexibility in the synthesis output.  This does mean it is important to check
spelling of the metadata, and create tests that the mapping is working as
expected.

Also note that "genfasm" will not accept "x" (unknown/don't care) or "z"
(high impedence) values in parameters.  Prior to emitting the eblif for place
and route, ensure that all parameters that will be mapped to FASM have a
valid "1" or "0".
