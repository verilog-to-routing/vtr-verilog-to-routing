# policy knobs for k6FracN10LB_mem20K_complexDSP_customSB_22nm (koios)
# sourced by the generic template after archSupportDir is set.
# dsp widths and ram abits come from arch_facts.tcl (generated from the xml).

set bramRomCost    0.5
# aggressive enough that borderline memories go hard rather than soft;
# koios brams are 20k so keep the same hard-bram bias as k6
set bramSpCost     30
set bramDpCost     100
set cmpLutWidth    6
# fracturable fle: sizes 1..5 cost 13 and size 6 costs 20
set lutCost        "5:13,6:20"
set hardAdderThreshold 3
set sweepMaxIters  64
# reuse the k6 abc scripts until a koios-tuned pair is justified
set abcOptScript   "$templateDir/k6_delay_gia_opt.scr"
set abcMapScript   "$templateDir/k6_delay.scr"
# builtins only; exotic complex-dsp models are appended from
# hardblock_keep_types.txt when stubAllHardblocks is on
set keepCellTypes  "t:multiply t:adder t:single_port_ram t:dual_port_ram"
# emit generic blackbox stubs for every exotic hardblock model in the arch
# so rtl under `complex_dsp` / `hard_mem` can passthrough by cell name
set stubAllHardblocks 1
