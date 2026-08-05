# policy knobs for k6_frac_N10_frac_chain_mem32K_40nm
# sourced by the generic template after archSupportDir is set.
# dsp widths and ram abits come from arch_facts.tcl (generated from the xml).

set bramRomCost    0.5
# aggressive enough that borderline memories go hard rather than soft
set bramSpCost     30
set bramDpCost     100
set cmpLutWidth    6
# fracturable fle  sizes 1..5 cost 13 and size 6 costs 20
set lutCost        "5:13,6:20"
set hardAdderThreshold 3
# keep $mul soft when both operand widths are at or below this
set minHardMulWidth 3
# minimum operand width for mul2dsp chunking (policy; facts set dspMaxWidth)
set dspMinWidth    2
set sweepMaxIters  64
set abcOptScript   "$templateDir/abc/delay_gia_opt.scr"
set abcMapScript   "$templateDir/abc/delay_map.scr"
set keepCellTypes  "t:multiply t:adder t:single_port_ram t:dual_port_ram"

# rebuild the abc scripts with vtr_flow/misc/mosaic/template/abc/build_delay_scr.py
