# VTR Architecture Description Guide

Read this before writing or modifying FPGA architecture XML files.

For the complete tag-by-tag specification, see `doc/src/arch/reference.rst`.
For annotated examples, see `doc/src/arch/example_arch.xml` and `libs/libarchfpga/arch/sample_arch.xml`.
Additional examples organized by feature are in `vtr_flow/arch/`.

## Reference Architectures

The following are the community's primary reference architectures. Study them before designing a new architecture — they represent years of accumulated modeling judgment:

| File | Description |
|------|-------------|
| `vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml` | Flagship 40nm heterogeneous arch: fracturable 6-LUT CLB, memory, and multiplier hard blocks. The most widely used baseline. |
| `vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml` | Koios 22nm arch: complex DSP blocks and custom switch blocks. Good reference for hard DSP modeling. |
| `vtr_flow/arch/titan/stratixiv_arch.timing.xml` | Stratix IV architecture capture. Reference for commercial-grade heterogeneous modeling. |
| `vtr_flow/arch/titan/stratix10_arch.timing.xml` | Stratix 10 architecture capture. Reference for more modern commercial modeling. |

## Modeling Concepts

These concepts must be understood before making structural decisions about an architecture. Modeling mistakes at this level are hard to fix after the fact.

### The Two Layers: Physical and Logical

A VTR architecture description has two distinct layers that serve different purposes in the CAD flow:

**Physical layer** — `<tiles>`, `<layout>`, `<device>`, `<switchlist>`, `<segmentlist>`:
- Describes the FPGA grid: which block types exist, how large they are, and where they appear
- Describes the global routing fabric: wire segments, switch blocks, connection blocks
- Controls pin locations (which edge of a tile each port appears on) and connection block density (`<fc>`)

**Logical layer** — `<models>`, `<complexblocklist>`:
- Describes the internal logic hierarchy of each block type using `<pb_type>` trees
- Specifies which primitives exist (LUTs, FFs, hard blocks) and how they interconnect internally
- Does not encode any physical position or layout information

The `<tiles>` section bridges the two layers: each `<sub_tile>` within a tile declares which `<pb_type>` (complex block) can be placed inside it via `<equivalent_sites>`. A tile can contain multiple sub-tile types to describe a heterogeneous tile (one that hosts different kinds of complex blocks at the same grid location).

### CAD Flow Order and Its Consequences

VTR's CAD flow runs in this order: **Pack → Place → Route**.

The packer runs first and has no knowledge of where on the device clusters will eventually be placed. It creates cluster instances of each logical block type (each top-level `<pb_type>`), but the placer assigns those clusters to physical tile locations later.

This has a critical implication: **complex blocks must be physically agnostic**. A complex block of type `X` can be placed in any tile that lists `X` as an equivalent site, anywhere on the device. The packer cannot reason about which tile location a cluster will land in, so it cannot distinguish between "left-side tile" and "right-side tile" when choosing a block type.

### Positional Invariance

When you define a tile type and place it on the grid, you are implicitly declaring that any complex block compatible with that tile can be placed in *any* instance of that tile, without restriction. This is **positional invariance**: the packer treats all tiles of a given type as interchangeable.

The corollary is that if you need different physical behavior at different grid locations (e.g., IO tiles on different chip edges), that variation must be handled in the **tile** layer, not in the complex block. The complex block stays uniform. A common mistake is creating separate `io_left` and `io_right` pb_types for side-specific IO — the packer cannot know at pack time which side a cluster will land on, so it cannot choose between them correctly. The right approach is one `io` pb_type whose internal logic is side-agnostic, with side-specific pin placement handled by `<pinlocations>` in the tile.

### Intra-Cluster vs. Global Routing

The `<interconnect>` tags inside `<pb_type>` nodes define **intra-cluster routing** — the connections between primitives within a single cluster. This routing is resolved by the packer: when the packer assembles atoms into a cluster, it determines which internal connections are used and verifies they are legal according to the `<pb_type>` hierarchy.

The channels, switch blocks, and connection blocks defined in `<segmentlist>` and `<switchlist>` define **global routing** — the connections between clusters across the device. This routing is resolved by the router, after placement is complete.

These two routing domains are entirely separate. Do not attempt to model intra-cluster connectivity using global routing constructs, or vice versa.

## File Structure

All architecture files use `<architecture>` as the root tag. The top-level sections must appear in this order:

```xml
<architecture>
  <models/>           <!-- Custom primitive models (BLIF .subckt types) -->
  <tiles/>            <!-- Tile-level port definitions (what the routing network sees) -->
  <layout/>           <!-- FPGA grid: which block type goes where -->
  <device/>           <!-- Physical device parameters (transistor sizes, area) -->
  <switchlist/>       <!-- Routing switch types (muxes, buffers) -->
  <segmentlist/>      <!-- Routing wire segment types -->
  <directlist/>       <!-- Optional: direct block-to-block connections -->
  <complexblocklist/> <!-- Detailed pb_type hierarchy (logic inside each tile) -->
</architecture>
```

## Models

Declares custom BLIF primitive types the architecture supports. Standard structures (`.names`, `.latch`, `.input`, `.output`) are implicit — do not declare them here. Each `<model>` contains `<input_ports>` and `<output_ports>`, each holding `<port>` tags. See the reference architectures above for complete examples.

Port attributes:
- `is_clock="1"` — marks a clock port
- `clock="clk"` — makes the port sequential (registered on the named clock)
- `combinational_sink_ports="a b"` — space-separated list of output ports with combinational paths from this input

## Tiles

Defines the ports visible at the tile boundary — what the routing network connects to. Each `<tile>` contains one or more `<sub_tile>` tags; each sub-tile holds the port definitions, `<equivalent_sites>`, `<fc>`, and `<pinlocations>`. See the reference architectures above for complete examples.

- `capacity` on `<sub_tile>` — how many sites of this type fit in the tile (e.g., multiple I/O pads per tile)
- `<equivalent_sites>` — links the sub-tile to its `<pb_type>`; `pin_mapping="direct"` means ports map by name
- `<fc>` — connection block density; `frac` is a fraction of channel width, `abs` is an absolute count
- `<pinlocations pattern="spread">` — distributes pins evenly across tile edges; `pattern="custom"` lets you assign specific edges per port

## Layout

Specifies where each block type appears on the FPGA grid. Use `<auto_layout>` for a scalable device or `<fixed_layout name="..." width="W" height="H">` for fixed dimensions.

Grid location tags are applied in priority order — higher priority overrides lower. All positions default to `EMPTY`.

Grid location tags:

| Tag | Meaning |
|-----|---------|
| `<fill type="X">` | Fill entire grid with X |
| `<perimeter type="X">` | Set all edge cells (including corners) |
| `<corners type="X">` | Set corner cells only |
| `<single type="X" x="?" y="?">` | Place one instance at (x, y) |
| `<col type="X" startx="?" repeatx="?">` | Column(s) of X |
| `<row type="X" starty="?" repeaty="?">` | Row(s) of X |
| `<region type="X" startx="?" endx="?" starty="?" endy="?">` | Rectangular region |

Position attributes accept expressions using `W` (device width), `H` (device height), `w` (block width), `h` (block height). All arithmetic is integer. Example: `startx="W/2 - w/2"` centers a block horizontally.

`<col>` and `<row>` have a `starty`/`startx` offset useful when a `<perimeter>` occupies the edge (set to `1` to avoid overlap). `repeatx`/`repeaty` repeats every N cells.

## Device

Physical parameters required by VPR's area model and routing. Required sub-tags: `<sizing>`, `<area>`, `<chan_width_distr>`, `<switch_block>`, `<connection_block>`. See the reference architectures above for values.

- `switch_block type` — `wilton`, `universal`, or `subset`; `fs` is the number of tracks each wire connects to at a switch block
- `connection_block input_switch_name` — must name a switch defined in `<switchlist>`

## Switchlist

Defines the electrical properties of routing switches. Each `<switch>` has attributes `type`, `name`, `R`, `Cin`, `Cout`, `Tdel`, `mux_trans_size`, and `buf_size`. See the reference architectures above for values.

Switch types: `mux` (buffered), `pass_gate` (unbuffered transistor), `short` (ideal zero-resistance wire), `buffer` (non-configurable buffer).

## Segmentlist

Defines routing wire segment types:

```xml
<segmentlist>
  <segment name="L4" length="4" freq="1.0" type="unidir"
           Rmetal="101" Cmetal="22.5e-15">
    <sb type="pattern">1 1 1 1 1</sb>  <!-- switch block tap at each position (length+1 values) -->
    <cb type="pattern">1 1 1 1</cb>    <!-- connection block tap at each position (length values) -->
    <mux name="0"/>                     <!-- switch used to drive this segment -->
  </segment>
</segmentlist>
```

- `type="unidir"` — driven by a buffer at one end; most modern architectures use this
- `type="bidir"` — pass transistors at each switch block; used in older/bidirectional architectures
- `freq` — relative proportion of routing tracks of this type (normalized across all segments)
- `<sb>` pattern — `1` means a switch block tap exists at that position along the wire; `0` means none
- `<cb>` pattern — same idea for connection block taps (one value per routing tile the wire spans)

## Directlist

Optional. Specifies direct connections between specific ports on adjacent tiles without going through the routing network (e.g., carry chains). Each `<direct>` names a `from_pin` and `to_pin` (as `TileName.port`) and an `x_offset`/`y_offset` tile-coordinate offset of the destination relative to the source.

## Complexblocklist

Defines the internal logic hierarchy of each block type using nested `<pb_type>` tags. This is the most complex section.

### pb_type Hierarchy

Top-level `<pb_type>` names must match the tile names in `<tiles>`. Intermediate `<pb_type>` nodes group children; leaf nodes (with `blif_model`) represent actual primitives.

```xml
<complexblocklist>
  <pb_type name="CLB">
    <input  name="I"   num_pins="33"/>
    <output name="O"   num_pins="10"/>
    <clock  name="clk" num_pins="1"/>

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

      <!-- Leaf: flip-flop -->
      <pb_type name="ff" blif_model=".latch" num_pb="1">
        <input  name="D"   num_pins="1"/>
        <output name="Q"   num_pins="1"/>
        <clock  name="clk" num_pins="1"/>
        <T_setup      value="66e-12"  port="ff.D" clock="clk"/>
        <T_clock_to_Q max="124e-12"  port="ff.Q" clock="clk"/>
      </pb_type>

      <interconnect>
        <direct name="lut_to_ff" input="lut_6.out" output="ff.D"/>
        <mux    name="ble_out"   input="lut_6.out ff.Q" output="ble.out"/>
      </interconnect>
    </pb_type>

    <interconnect>
      <complete name="crossbar" input="CLB.I ble[9:0].out" output="ble[9:0].in"/>
      <complete name="clks"     input="CLB.clk"            output="ble[9:0].clk"/>
      <direct   name="clb_out"  input="ble[9:0].out"       output="CLB.O"/>
    </interconnect>
  </pb_type>
</complexblocklist>
```

**Leaf node `blif_model` values:**
- `.names` — LUT
- `.latch` — flip-flop
- `.input` — top-level input pad
- `.output` — top-level output pad
- `".subckt model_name"` — custom model declared in `<models>`

### Interconnect

`<interconnect>` appears inside each non-leaf `<pb_type>` and specifies how ports connect:

| Tag | Meaning |
|-----|---------|
| `<direct name="..." input="A.x" output="B.y"/>` | Point-to-point or bus (widths must match) |
| `<mux name="..." input="A.x B.y" output="C.z"/>` | Programmable mux; output selects one input |
| `<complete name="..." input="A.x B.y" output="C.z"/>` | Full crossbar: every input bit connects to every output bit |

Port references use `pb_type_name.port_name` notation. Ranges use `pb_type_name[high:low].port_name`.

### Modes

When a block can operate in multiple configurations, use `<mode>` tags. A `<pb_type>` with modes has no direct children — all children live inside the modes. The flagship and Koios architectures both use modes for fracturable LUTs; refer to them for complete examples.

### Pack Patterns

Tell the packer which primitives should be packed together into the same `<pb_type>` instance. Patterns are named and must appear consistently on both sides of a connection: on the source port's `<output>` tag and on the sink port's `<input>` tag, each referencing the other via `in_port` and `out_port`. Both must name sibling `<pb_type>` ports at the same hierarchy level. See the reference architectures for complete examples.

## Timing Modeling

All timing values are in seconds. See the reference architectures for complete examples.

**Combinational delays** use `<delay_matrix>` (one value per input pin, space-separated) or `<delay_constant max="..." in_port="..." out_port="..."/>`.

**Sequential primitives** use `<T_setup value="..." port="..." clock="..."/>`, `<T_hold>`, and `<T_clock_to_Q max="..." port="..." clock="..."/>`.

## Validation

To check an architecture file, run VPR with a small benchmark circuit:

```bash
source .venv/bin/activate
./vpr/vpr <arch.xml> vtr_flow/benchmarks/blif/and.blif --route_chan_width 100
```

VPR reports the exact location of any XML error (mismatched widths, missing tags, unknown attributes). The `and.blif` benchmark is the simplest available test circuit.

## Common Mistakes

- **Missing model declaration** — any `.subckt` model used in `<complexblocklist>` must be declared in `<models>` first
- **Mismatched port widths in interconnect** — `<direct>` requires identical widths on input and output; VPR reports the exact mismatch
- **`<complete>` vs `<direct>`** — `<complete>` creates an N×M full crossbar; use `<direct>` for 1-to-1 or bus connections
- **Missing `<equivalent_sites>`** — every `<sub_tile>` requires this to link it to its `<pb_type>`
- **`<pack_pattern>` port references** — `in_port`/`out_port` must name ports on sibling `<pb_type>` nodes at the same hierarchy level, not on parent or child nodes
- **`starty` offset missing with `<perimeter>`** — when using `<col>` or `<row>` alongside `<perimeter>`, set `starty="1"` (or `startx="1"`) to avoid overlapping the perimeter tiles
- **Multi-die layout** — all dice share the same width and height; use `EMPTY` to leave areas of a die unused
