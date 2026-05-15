---
name: vtr-architect
description: Use this skill when the user wants to write, modify, or understand VTR FPGA architecture description XML files. Activate whenever the user mentions FPGA architecture modeling, architecture XML, pb_type, tile description, routing segments, switch blocks, CLB, logic block, memory block, DSP block, or asks how to describe a specific FPGA component in VTR. Also activate when the user asks questions like "how do I model X in VTR" or "what does this architecture tag mean".
---

# VTR Architecture Skill

This skill helps you write and understand VTR's XML-based FPGA architecture description language.

## Key References

Before writing or modifying architecture files, read the relevant reference material:

- **Full reference**: `doc/src/arch/reference.rst` — complete tag-by-tag specification
- **Annotated example**: `doc/src/arch/example_arch.xml` — fully commented reference arch with CLB, RAM, and multiplier blocks
- **Simple example**: `libs/libarchfpga/arch/sample_arch.xml` — flagship 40nm heterogeneous arch, good starting point
- **Common arch**: `vtr_flow/arch/common/arch.xml` — minimal arch used in regression tests
- **More examples**: `vtr_flow/arch/` — organized by feature (sub_tiles, equivalent_sites, noc, multi_die, etc.)

The full reference at `doc/src/arch/reference.rst` is long (3300+ lines). Use it as a lookup when you need exact attribute names and defaults — search for the specific tag you need rather than reading it top to bottom.

## Architecture File Structure

All architecture files start with `<architecture>` and contain these top-level sections (order matters):

```xml
<architecture>
  <models/>           <!-- Custom primitive models (BLIF .subckt types) -->
  <tiles/>            <!-- Tile-level port definitions (what the routing sees) -->
  <layout/>           <!-- FPGA grid: which block goes where -->
  <device/>           <!-- Physical device parameters (transistor sizes, area) -->
  <switchlist/>       <!-- Routing switches (muxes/buffers) -->
  <segmentlist/>      <!-- Routing wire segments -->
  <directlist/>       <!-- Optional: direct block-to-block connections -->
  <complexblocklist/> <!-- Detailed pb_type hierarchy (the logic inside tiles) -->
</architecture>
```

## Core Concepts

### Models (`<models>`)

Declares custom BLIF primitives the architecture supports. Standard structures (`.names`, `.latch`, `.input`, `.output`) are implicit — do not declare them.

```xml
<models>
  <model name="single_port_ram">
    <input_ports>
      <port name="we"   clock="clk"/>
      <port name="addr" clock="clk" combinational_sink_ports="out"/>
      <port name="data" clock="clk" combinational_sink_ports="out"/>
      <port name="clk"  is_clock="1"/>
    </input_ports>
    <output_ports>
      <port name="out" clock="clk"/>
    </output_ports>
  </model>
</models>
```

Key port attributes:
- `is_clock="1"` — marks a clock port
- `clock="clk"` — makes port sequential (registered), controlled by named clock
- `combinational_sink_ports="a b"` — lists output ports with combinational paths from this input

### Tiles (`<tiles>`)

Defines the ports visible at the top-level tile boundary — what the routing network connects to. Each tile maps to one `<pb_type>` in the complexblocklist.

```xml
<tiles>
  <tile name="CLB">
    <sub_tile name="CLB" capacity="1">
      <equivalent_sites>
        <site pb_type="CLB" pin_mapping="direct"/>
      </equivalent_sites>
      <input  name="I"   num_pins="33"/>
      <output name="O"   num_pins="10"/>
      <clock  name="clk" num_pins="1"/>
      <fc in_type="frac" in_val="0.15" out_type="frac" out_val="0.10"/>
      <pinlocations pattern="spread"/>
    </sub_tile>
  </tile>
</tiles>
```

`<fc>` controls connection block density — fraction of routing tracks connected to each pin. `frac` means fraction of channel width; `abs` means absolute count.

`<pinlocations>` places pins on tile edges. `pattern="spread"` distributes evenly; `pattern="custom"` lets you specify which side each pin appears on.

### Layout (`<layout>`)

Specifies the FPGA grid. Use `<auto_layout>` for a scalable device or `<fixed_layout>` for fixed dimensions. Grid location tags are applied in priority order (higher priority wins):

```xml
<layout>
  <auto_layout aspect_ratio="1.0">
    <perimeter type="io"    priority="10"/>
    <corners   type="EMPTY" priority="100"/>
    <col       type="RAM"   startx="2" repeatx="8" starty="1" priority="3"/>
    <fill      type="CLB"   priority="1"/>
  </auto_layout>
</layout>
```

Grid location tags: `<fill>`, `<perimeter>`, `<corners>`, `<single x="?" y="?">`, `<col startx="?" repeatx="?">`, `<row starty="?" repeaty="?">`, `<region startx="?" endx="?" starty="?" endy="?">`.

Expressions in position attributes can use `W` (device width), `H` (device height), `w` (block width), `h` (block height).

### Device (`<device>`)

Physical parameters for the area model and routing. Required tags:

```xml
<device>
  <sizing R_minW_nmos="6065.520" R_minW_pmos="18138.500"/>
  <area grid_logic_tile_area="14813.392"/>
  <chan_width_distr>
    <x distr="uniform" peak="1.0"/>
    <y distr="uniform" peak="1.0"/>
  </chan_width_distr>
  <switch_block type="wilton" fs="3"/>
  <connection_block input_switch_name="ipin_cblock"/>
</device>
```

`switch_block type` options: `wilton`, `universal`, `subset`. `fs` is the switch block flexibility (number of connections per wire). For `type="custom"`, you define it explicitly in `<switchlist>`.

### Switchlist (`<switchlist>`)

Defines routing switch types used in connection blocks and switch blocks:

```xml
<switchlist>
  <switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15"
          Tdel="58e-12" mux_trans_size="2.630" buf_size="27.645"/>
  <switch type="pass_gate" name="ipin_cblock" R="2231.5" Cin=".5e-15"
          Cout="0" Tdel="6.837e-12" mux_trans_size="1.222" buf_size="0"/>
</switchlist>
```

Switch types: `mux` (buffered), `pass_gate` (unbuffered), `short` (ideal wire), `buffer`.

### Segmentlist (`<segmentlist>`)

Defines routing wire segments:

```xml
<segmentlist>
  <segment name="L4" length="4" freq="1.0" type="unidir"
           Rmetal="101" Cmetal="22.5e-15">
    <sb type="pattern">1 1 1 1 1</sb>  <!-- switch block connections at each position -->
    <cb type="pattern">1 1 1 1</cb>    <!-- connection block connections -->
    <mux name="0"/>
  </segment>
</segmentlist>
```

`type="unidir"` (unidirectional, driven by a buffer at one end) vs `type="bidir"` (bidirectional, pass transistors at each switch block). Most modern architectures use `unidir`.

### Complex Block List (`<complexblocklist>`)

Defines the internal hierarchy of each block type using nested `<pb_type>` tags. This is the most complex section.

```xml
<complexblocklist>
  <pb_type name="CLB">
    <input  name="I"   num_pins="33"/>
    <output name="O"   num_pins="10"/>
    <clock  name="clk" num_pins="1"/>

    <!-- Child pb_types (BLEs inside CLB) -->
    <pb_type name="ble" num_pb="10">
      <input  name="in"  num_pins="6"/>
      <output name="out" num_pins="1"/>
      <clock  name="clk" num_pins="1"/>

      <!-- Leaf: LUT -->
      <pb_type name="lut_6" blif_model=".names" num_pb="1">
        <input  name="in"  num_pins="6"/>
        <output name="out" num_pins="1"/>
        <delay_matrix type="max" in_port="lut_6.in" out_port="lut_6.out">
          261e-12 261e-12 261e-12 261e-12 261e-12 261e-12
        </delay_matrix>
      </pb_type>

      <!-- Leaf: FF -->
      <pb_type name="ff" blif_model=".latch" num_pb="1">
        <input  name="D"   num_pins="1"/>
        <output name="Q"   num_pins="1"/>
        <clock  name="clk" num_pins="1"/>
        <T_setup value="66e-12" port="ff.D" clock="clk"/>
        <T_clock_to_Q max="124e-12" port="ff.Q" clock="clk"/>
      </pb_type>

      <!-- Internal routing -->
      <interconnect>
        <direct name="lut_to_ff" input="lut_6.out" output="ff.D"/>
        <mux name="ble_out" input="lut_6.out ff.Q" output="ble.out"/>
      </interconnect>
    </pb_type>

    <!-- Crossbar: CLB inputs to BLE inputs -->
    <interconnect>
      <complete name="crossbar" input="CLB.I ble[9:0].out" output="ble[9:0].in"/>
      <complete name="clks"     input="CLB.clk"            output="ble[9:0].clk"/>
      <direct   name="clb_out"  input="ble[9:0].out"       output="CLB.O"/>
    </interconnect>
  </pb_type>
</complexblocklist>
```

**Leaf nodes** have `blif_model` set: `.names` (LUT), `.latch` (FF), `.input`, `.output`, or `".subckt model_name"` for custom models.

**Interconnect types**:
- `<direct>` — point-to-point, or bus (must match widths)
- `<mux>` — selects one of several inputs; size inferred from input count
- `<complete>` — every input connected to every output (full crossbar)

**Pack patterns** (`<pack_pattern>`) on leaf ports tell the packer which primitives must be packed together:

```xml
<pb_type name="ff" blif_model=".latch" num_pb="1">
  <input name="D" num_pins="1">
    <pack_pattern name="ble" in_port="lut_6.out" out_port="ff.D"/>
  </input>
  ...
</pb_type>
```

## Timing Modeling

**Combinational primitives** use `<delay_matrix>` or `<delay_constant>`:
```xml
<delay_matrix type="max" in_port="lut_6.in" out_port="lut_6.out">
  261e-12 261e-12 261e-12 261e-12 261e-12 261e-12
</delay_matrix>
```

**Sequential primitives** use setup/hold and clock-to-Q:
```xml
<T_setup       value="66e-12"  port="ff.D" clock="clk"/>
<T_hold        value="0"       port="ff.D" clock="clk"/>
<T_clock_to_Q  max="124e-12"   port="ff.Q" clock="clk"/>
```

## Fracturable LUTs

A 6-LUT that can operate as two 5-LUTs uses `<mode>` tags:

```xml
<pb_type name="fle" num_pb="1">
  <mode name="n1_lut6">  <!-- 6-LUT mode -->
    <pb_type name="lut6" blif_model=".names" num_pb="1">...</pb_type>
    <interconnect>...</interconnect>
  </mode>
  <mode name="n2_lut5">  <!-- Two 5-LUT mode -->
    <pb_type name="lut5" blif_model=".names" num_pb="2">...</pb_type>
    <interconnect>...</interconnect>
  </mode>
</pb_type>
```

## Validation

To validate an architecture file, run VPR with any benchmark circuit:

```bash
source .venv/bin/activate
./vpr/vpr <arch.xml> <circuit.blif> --route_chan_width 100
```

For a quick schema check without routing, use:
```bash
./vpr/vpr <arch.xml> <circuit.blif> --pack --place --route_chan_width 100 --device_layout 5x5
```

Small test circuits are in `vtr_flow/benchmarks/blif/`. The simplest is `vtr_flow/benchmarks/blif/and.blif`.

## Common Mistakes

- Forgetting to declare a custom model in `<models>` before referencing it in `<complexblocklist>`
- Mismatched port widths in `<interconnect>` — VPR will report the exact mismatch
- Using `<complete>` when you need `<direct>` (complete creates NxM connections, direct creates 1:1)
- Missing `<equivalent_sites>` in a `<sub_tile>` — required to link tile to its pb_type
- `<pack_pattern>` `in_port`/`out_port` must name ports on sibling pb_types at the same hierarchy level
- Grid layout: all dice have the same width/height; use `EMPTY` to leave areas unused
