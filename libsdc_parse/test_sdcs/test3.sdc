#VPR compatible SDC file for benchmark 'mes_noc'

#*******************************
# set_time_format 
#*******************************
# Unsuported by VPR

#*******************************
# create_clock 
#*******************************
create_clock -period 1.0 -name virtual_io_clock
create_clock -period 1.0 clk
create_clock -period 1.0 {pll_noc_type0:\using_pll:separate_clk:noc_pll_0|altpll:altpll_component|pll_noc_type0_altpll:auto_generated|wire_pll1_clk[0]}
create_clock -period 1.0 {pll_noc_type1:\using_pll:use_noc_pll_1:noc_pll_1|altpll:altpll_component|pll_noc_type1_altpll:auto_generated|wire_pll1_clk[0]}
create_clock -period 1.0 {pll_noc_type2:\using_pll:use_noc_pll_2:noc_pll_2|altpll:altpll_component|pll_noc_type2_altpll:auto_generated|wire_pll1_clk[0]}
create_clock -period 1.0 {pll_noc_type5:\using_pll:use_noc_pll_5:noc_pll_5|altpll:altpll_component|pll_noc_type5_altpll:auto_generated|wire_pll1_clk[0]}
create_clock -period 1.0 {pll_noc_type4:\using_pll:use_noc_pll_4:noc_pll_4|altpll:altpll_component|pll_noc_type4_altpll:auto_generated|wire_pll1_clk[0]}
create_clock -period 1.0 {pll_noc_type3:\using_pll:use_noc_pll_3:noc_pll_3|altpll:altpll_component|pll_noc_type3_altpll:auto_generated|wire_pll1_clk[0]}
create_clock -period 1.0 {pll_noc_type6:\using_pll:use_noc_pll_6:noc_pll_6|altpll:altpll_component|pll_noc_type6_altpll:auto_generated|wire_pll1_clk[0]}
create_clock -period 1.0 {pll_noc_type7:\using_pll:use_noc_pll_7:noc_pll_7|altpll:altpll_component|pll_noc_type7_altpll:auto_generated|wire_pll1_clk[0]}

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
set_clock_groups -exclusive -group { clk } -group { pll_noc_type0:\using_pll:separate_clk:noc_pll_0|altpll:altpll_component|pll_noc_type0_altpll:auto_generated|wire_pll1_clk[0] } -group { pll_noc_type1:\using_pll:use_noc_pll_1:noc_pll_1|altpll:altpll_component|pll_noc_type1_altpll:auto_generated|wire_pll1_clk[0] } -group { pll_noc_type2:\using_pll:use_noc_pll_2:noc_pll_2|altpll:altpll_component|pll_noc_type2_altpll:auto_generated|wire_pll1_clk[0] } -group { pll_noc_type5:\using_pll:use_noc_pll_5:noc_pll_5|altpll:altpll_component|pll_noc_type5_altpll:auto_generated|wire_pll1_clk[0] } -group { pll_noc_type4:\using_pll:use_noc_pll_4:noc_pll_4|altpll:altpll_component|pll_noc_type4_altpll:auto_generated|wire_pll1_clk[0] } -group { pll_noc_type3:\using_pll:use_noc_pll_3:noc_pll_3|altpll:altpll_component|pll_noc_type3_altpll:auto_generated|wire_pll1_clk[0] } -group { pll_noc_type6:\using_pll:use_noc_pll_6:noc_pll_6|altpll:altpll_component|pll_noc_type6_altpll:auto_generated|wire_pll1_clk[0] } -group { pll_noc_type7:\using_pll:use_noc_pll_7:noc_pll_7|altpll:altpll_component|pll_noc_type7_altpll:auto_generated|wire_pll1_clk[0] }
