.. _virtex_6_like_logic_slice_example:

Virtex 6 like Logic Slice Example
---------------------------------

In order to demonstrate the expressiveness of the architecture description language, we use it to describe a section of a commercial logic block.
In this example, we describe the Xilinx Virtex-6 FPGA logic slice :cite:`xilinx_virtex_6_clb`, shown in :numref:`fig_v6_slice`, as follows:

.. _fig_v6_slice:

.. figure:: v6_logic_slice.*

    Commercial FPGA logic block slice (Xilinx Virtex-6)

.. code-block:: xml

    <pb_type name="v6_lslice">

      <input name="AX" num_pins="1" equivalent="false"/>
      <input name="A" num_pins="5" equivalent="false"/>
      <input name="AI" num_pins="1" equivalent="false"/>
      <input name="BX" num_pins="1" equivalent="false"/>
      <input name="B" num_pins="5" equivalent="false"/>
      <input name="BI" num_pins="1" equivalent="false"/>
      <input name="CX" num_pins="1" equivalent="false"/>
      <input name="C" num_pins="5" equivalent="false"/>
      <input name="CI" num_pins="1" equivalent="false"/>
      <input name="DX" num_pins="1" equivalent="false"/>
      <input name="D" num_pins="5" equivalent="false"/>
      <input name="DI" num_pins="1" equivalent="false"/>
      <input name="SR" num_pins="1" equivalent="false"/>
      <input name="CIN" num_pins="1" equivalent="false"/>
      <input name="CE" num_pins="1" equivalent="false"/>

      <output name="AMUX" num_pins="1" equivalent="false"/>
      <output name="Aout" num_pins="1" equivalent="false"/>
      <output name="AQ" num_pins="1" equivalent="false"/>
      <output name="BMUX" num_pins="1" equivalent="false"/>
      <output name="Bout" num_pins="1" equivalent="false"/>
      <output name="BQ" num_pins="1" equivalent="false"/>
      <output name="CMUX" num_pins="1" equivalent="false"/>
      <output name="Cout" num_pins="1" equivalent="false"/>
      <output name="CQ" num_pins="1" equivalent="false"/>
      <output name="DMUX" num_pins="1" equivalent="false"/>
      <output name="Dout" num_pins="1" equivalent="false"/>
      <output name="DQ" num_pins="1" equivalent="false"/>
      <output name="COUT" num_pins="1" equivalent="false"/>

      <clock name="CLK"/>

      <!--
        For the purposes of this example, the Virtex-6 fracturable LUT will be specified as a primitive.
        If the architect wishes to explore the Xilinx Virtex-6 further, add more detail into this pb_type.
        Similar convention for flip-flops
      -->
      <pb_type name="fraclut" num_pb="4" blif_model=".subckt vfraclut">
        <input name="A" num_pins="5"/>
        <input name="W" num_pins="5"/>
        <input name="DI1" num_pins="1"/>
        <input name="DI2" num_pins="1"/>
        <output name="MC31" num_pins="1"/>
        <output name="O6" num_pins="1"/>
        <output name="O5" num_pins="1"/>
      </pb_type>
      <pb_type name="carry" num_pb="4" blif_model=".subckt carry">
        <!-- This is actually the carry-chain but we don't have a special way to specify chain logic yet in UTFAL
             so it needs to be specified as regular gate logic, the xor gate and the two muxes to the left of it that are shaded grey
             comprise the logic gates representing the carry logic -->
        <input name="xor" num_pins="1"/>
        <input name="cmuxxor" num_pins="1"/>
        <input name="cmux" num_pins="1"/>
        <input name="cmux_select" num_pins="1"/>
        <input name="mmux" num_pins="2"/>
        <input name="mmux_select" num_pins="1"/>
        <output name="xor_out" num_pins="1"/>
        <output name="cmux_out" num_pins="1"/>
        <output name="mmux_out" num_pins="1"/>
      </pb_type>
      <pb_type name="ff_small" num_pb="4" blif_model=".subckt vffs">
        <input name="D" num_pins="1"/>
        <input name="CE" num_pins="1"/>
        <input name="SR" num_pins="1"/>
        <output name="Q" num_pins="1"/>
        <clock name="CK" num_pins="1"/>
      </pb_type>
      <pb_type name="ff_big" num_pb="4" blif_model=".subckt vffb">
        <input name="D" num_pins="1"/>
        <input name="CE" num_pins="1"/>
        <input name="SR" num_pins="1"/>
        <output name="Q" num_pins="1"/>
        <clock name="CK" num_pins="1"/>
      </pb_type>
      <!-- TODO: Add in ability to specify constants such as gnd/vcc -->

      <interconnect>
        <direct name="fraclutA" input="{v6_lslice.A v6_lslice.B v6_lslice.C v6_lslice.D}" output="fraclut.A"/>
        <direct name="fraclutW" input="{v6_lslice.A v6_lslice.B v6_lslice.C v6_lslice.D}" output="fraclut.W"/>
        <direct name="fraclutDI2" input="{v6_lslice.AX v6_lslice.BX v6_lslice.CX v6_lslice.DX}" output="fraclut.DI2"/>
        <direct name="DfraclutDI1" input="v6_lslice.DI" output="fraclut[3].DI1"/>

        <direct name="carryO6" input="fraclut.O6" output="carry.xor"/>
        <direct name="carrymuxxor" input="carry[2:0].cmux_out" output="carry[3:1].cmuxxor"/>
        <direct name="carrymmux" input="{fraclut[3].O6 fraclut[2].O6 fraclut[2].O6 fraclut[1].O6 fraclut[1].O6 fraclut[0].O6}" output="carry[2:0].mmux"/>
        <direct name="carrymmux_select" input="{v6_lslice.AX v6_lslice.BX v6_lslice.CX}" output="carry[2:0].mmux_select"/>

        <direct name="cout" input="carry[3].mmux_out" output="v6_lslice.COUT"/>
        <direct name="ABCD" input="fraclut[3:0].O6" output="{v6_lslice.Dout v6_lslice.Cout v6_lslice.Bout v6_lslice.Aout}"/>
        <direct name="Q" input="ff_big.Q" output="{DQ CQ BQ AQ}"/>

        <mux name="ff_smallA" input="v6_lslice.AX fraclut[0].O5" output="ff_small[0].D"/>
        <mux name="ff_smallB" input="v6_lslice.BX fraclut[1].O5" output="ff_small[1].D"/>
        <mux name="ff_smallC" input="v6_lslice.CX fraclut[2].O5" output="ff_small[2].D"/>
        <mux name="ff_smallD" input="v6_lslice.DX fraclut[3].O5" output="ff_small[3].D"/>

        <mux name="ff_bigA" input="fraclut[0].O5 fraclut[0].O6 carry[0].cmux_out carry[0].mmux_out carry[0].xor_out" output="ff_big[0].D"/>
        <mux name="ff_bigB" input="fraclut[1].O5 fraclut[1].O6 carry[1].cmux_out carry[1].mmux_out carry[1].xor_out" output="ff_big[1].D"/>
        <mux name="ff_bigC" input="fraclut[2].O5 fraclut[2].O6 carry[2].cmux_out carry[2].mmux_out carry[2].xor_out" output="ff_big[2].D"/>
        <mux name="ff_bigD" input="fraclut[3].O5 fraclut[3].O6 carry[3].cmux_out carry[3].mmux_out carry[3].xor_out" output="ff_big[3].D"/>

        <mux name="AMUX" input="fraclut[0].O5 fraclut[0].O6 carry[0].cmux_out carry[0].mmux_out carry[0].xor_out ff_small[0].Q" output="AMUX"/>
        <mux name="BMUX" input="fraclut[1].O5 fraclut[1].O6 carry[1].cmux_out carry[1].mmux_out carry[1].xor_out ff_small[1].Q" output="BMUX"/>
        <mux name="CMUX" input="fraclut[2].O5 fraclut[2].O6 carry[2].cmux_out carry[2].mmux_out carry[2].xor_out ff_small[2].Q" output="CMUX"/>
        <mux name="DMUX" input="fraclut[3].O5 fraclut[3].O6 carry[3].cmux_out carry[3].mmux_out carry[3].xor_out ff_small[3].Q" output="DMUX"/>

        <mux name="CfraclutDI1" input="v6_lslice.CI v6_lslice.DI fraclut[3].MC31" output="fraclut[2].DI1"/>
        <mux name="BfraclutDI1" input="v6_lslice.BI v6_lslice.DI fraclut[2].MC31" output="fraclut[1].DI1"/>
        <mux name="AfraclutDI1" input="v6_lslice.AI v6_lslice.BI v6_lslice.DI fraclut[2].MC31 fraclut[1].MC31" output="fraclut[0].DI1"/>

        <mux name="carrymuxxorA" input="v6_lslice.AX v6_lslice.CIN" output="carry[0].muxxor"/>
        <mux name="carrymuxA" input="v6_lslice.AX fraclut[0].O5" output="carry[0].cmux"/>
        <mux name="carrymuxB" input="v6_lslice.BX fraclut[1].O5" output="carry[1].cmux"/>
        <mux name="carrymuxC" input="v6_lslice.CX fraclut[2].O5" output="carry[2].cmux"/>
        <mux name="carrymuxD" input="v6_lslice.DX fraclut[3].O5" output="carry[3].cmux"/>


        <complete name="clock" input="v6_lslice.CLK" output="{ff_small.CK ff_big.CK}"/>
        <complete name="ce" input="v6_lslice.CE" output="{ff_small.CE ff_big.CE}"/>
        <complete name="SR" input="v6_lslice.SR" output="{ff_small.SR ff_big.SR}"/>
      </interconnect>
    </pb_type>
