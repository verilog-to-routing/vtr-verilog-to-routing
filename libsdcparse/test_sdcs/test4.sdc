#VPR compatible SDC file for benchmark 'bitcoin_miner'

#*******************************
# set_time_format 
#*******************************
# Unsuported by VPR

#*******************************
# create_clock 
#*******************************
create_clock -period 1.0 -name virtual_io_clock
create_clock -period 1.0 {pll:pll_inst|altpll:altpll_component|pll_altpll:auto_generated|wire_pll1_clk[0]}
create_clock -period 1.0 OSC_CLK

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
set_clock_groups -exclusive -group { pll:pll_inst|altpll:altpll_component|pll_altpll:auto_generated|wire_pll1_clk[0] } -group { OSC_CLK }
