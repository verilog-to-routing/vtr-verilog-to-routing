# this file is the mosaic policy for k6FracN10LB_mem20K_complexDSP_customSB_22nm.
# synthesis.tcl sources it after archSupportDir is set, while dsp widths and ram
# abits still come from arch_facts.tcl which is generated from the arch xml.

set bramRomCost    0.5
# this arch has 20k brams, so the costs bias borderline memories toward hard bram.
set bramSpCost     30
set bramDpCost     100
set cmpLutWidth    6
# fracturable fle sizes 1 through 5 cost 13 and size 6 costs 20 so abc sees the split.
set lutCost        "5:13,6:20"
set hardAdderThreshold 3
# $mul stays soft when both operand widths are at or below this threshold so tiny
# products do not burn a hard dsp.
set minHardMulWidth 3
# dspMinWidth is the mul2dsp chunking floor, while facts still supply dspMaxWidth.
set dspMinWidth    2
set sweepMaxIters  64
# shared delay abc scripts stay in use until an arch tuned pair is justified.
set abcOptScript   "$templateDir/abc/delay_gia_opt.scr"
set abcMapScript   "$templateDir/abc/delay_map.scr"
# keep the classic cells first, and exotic complex dsp models are appended from
# hardblock_keep_types.txt when stubAllHardblocks is on.
set keepCellTypes  "t:multiply t:adder t:single_port_ram t:dual_port_ram"
# every exotic hardblock is blackboxed so rtl under complex_dsp or hard_mem can
# passthrough by cell name without mosaic inventing a techmap.
set stubAllHardblocks 1
# exoticRoles stays empty so classic multiply keeps inferred $mul, and roles are
# only set for models whose ports match a stock role such as integer_mul.
set exoticRoles {}
set exoticTemplatePairs {}
