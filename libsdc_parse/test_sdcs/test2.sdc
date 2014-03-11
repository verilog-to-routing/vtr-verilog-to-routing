#VPR compatible SDC file for benchmark 'minres'

#*******************************
# set_time_format 
#*******************************
# Unsuported by VPR

#*******************************
# create_clock 
#*******************************
create_clock -period 1.0 -name virtual_io_clock
create_clock -period 1.0 clk0
create_clock -period 1.0 { a[0] }
create_clock -period 1.0 {pll_minres:inst_pll_minres|altpll:altpll_component|pll_minres_altpll:auto_generated|wire_pll1_clk[0]}

#*******************************
# set_clock_uncertainty 
#*******************************
# Unsupported by VPR.  VPR does not model clock uncertainty.

#*******************************
# set_input_delay 
#*******************************
set_input_delay -clock virtual_io_clock -max 0.0 [get_ports *]

#*******************************
# set_output_delay 
#*******************************
set_output_delay -clock virtual_io_clock -max 0.0 [get_ports *]

#*******************************
# set_clock_groups 
#*******************************
set_clock_groups -exclusive -group { clk0 } -group { pll_minres:inst_pll_minres|altpll:altpll_component|pll_minres_altpll:auto_generated|wire_pll1_clk[0] }
