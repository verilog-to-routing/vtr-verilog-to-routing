# policy knobs for k6FracN10LB_mem20K_complexDSP_customSB_22nm
# sourced by the generic template after archSupportDir is set.
# dsp widths and ram abits come from arch_facts.tcl (generated from the xml).

set bramRomCost    0.5
# aggressive enough that borderline memories go hard rather than soft;
# this arch's brams are 20k so keep a hard-bram bias
set bramSpCost     30
set bramDpCost     100
set cmpLutWidth    6
# fracturable fle: sizes 1..5 cost 13 and size 6 costs 20
set lutCost        "5:13,6:20"
set hardAdderThreshold 3
# minimum operand width for mul2dsp chunking (policy; facts set dspMaxWidth)
set dspMinWidth    2
set sweepMaxIters  64
# reuse the shared delay abc scripts until an arch-tuned pair is justified
set abcOptScript   "$templateDir/abc/delay_gia_opt.scr"
set abcMapScript   "$templateDir/abc/delay_map.scr"
# builtins only; exotic complex-dsp models are appended from
# hardblock_keep_types.txt when stubAllHardblocks is on
set keepCellTypes  "t:multiply t:adder t:single_port_ram t:dual_port_ram"
# emit generic blackbox stubs for every exotic hardblock model in the arch
# so rtl under `complex_dsp` / `hard_mem` can passthrough by cell name
set stubAllHardblocks 1
# role inference (see model_roles.example.tcl); leave empty so classic
# multiply keeps $mul. enable only for models with matching port geometry.
set exoticRoles {}
# set exoticRoles {{my_mul integer_mul}}
set exoticTemplatePairs {}
