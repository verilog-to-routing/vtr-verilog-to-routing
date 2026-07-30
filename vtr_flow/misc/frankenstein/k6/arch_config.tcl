# arch_config.tcl for k6_frac_N10_frac_chain_mem32K_40nm
# sourced by the generic template after archSupportDir is set
# docs/doc-general-synthesis-template.md has the full knob list

set dspMaxWidth    18
set dspMinWidth    2
set bramRomCost    0.5
# aggressive enough that borderline memories go hard rather than soft
set bramSpCost     30
set bramDpCost     100
set cmpLutWidth    6
# fracturable fle  sizes 1..5 cost 13 and size 6 costs 20
set lutCost        "5:13,6:20"
set sweepMaxIters  64
set abcOptScript   "$templateDir/k6_delay_gia_opt.scr"
set abcMapScript   "$templateDir/k6_delay.scr"
set keepCellTypes  "t:multiply t:adder t:single_port_ram t:dual_port_ram"

# rebuild the abc scripts with vtr_flow/misc/frankenstein/template/build_k6_delay_scr.py
