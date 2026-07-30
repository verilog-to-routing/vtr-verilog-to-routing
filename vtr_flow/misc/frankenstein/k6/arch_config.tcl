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
# 12 under-mapped real datapath adds on k6 at the old higher cutoff
set hardAdderThreshold 3
set sweepMaxIters  64
set keepCellTypes  "t:multiply t:adder t:single_port_ram t:dual_port_ram"
