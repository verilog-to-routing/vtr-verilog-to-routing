#
# Clocks
#

#Netlist clocks
create_clock -period 3 -waveform {1.25 2.75} clk
create_clock -period 2 clk2

#Virtual Clocks
create_clock -period 1 -name input_clk
create_clock -period 0 -name output_clk

#
#Clock uncertainties
#

#Set 25ps clock uncertainty between all clocks
set_clock_uncertainty 0.025

#Override uncertainty from input_clk
set_clock_uncertainty 0.050 -from [get_clocks{clk}]

#
#Clock latencies
#

#Set a default clock latency of 500ps for all clocks
set_clock_latency -source 0.500 [get_clocks {*}]

#Set a different latency for the input clock
set_clock_latency -source 0.750 [get_clocks {input_clk}]

#Set a different latency for the output clock
set_clock_latency -source 0.850 [get_clocks {output_clk}]

#
# False paths
#

#No paths to/from input_clk and clk2
set_clock_groups -exclusive -group [get_clocks {input_clk}] -group [get_clocks {clk2}]

#No paths from clk to output_clk
set_false_path -from [get_clocks{clk}] -to [get_clocks{output_clk}]

#
# Multicycle paths
#

#Allow 3 cycles for transfers from clk to clk2
set_multicycle_path -setup -from [get_clocks{clk}] -to [get_clocks{clk2}] 3

#Keep the hold check at 0 cycles
set_multicycle_path -hold -from [get_clocks{clk}] -to [get_clocks{clk2}] 2

#
# Delay overrides
#

#Override the default clock constraints from input_clk to output_clk
set_max_delay 17.0 -from [get_clocks{input_clk}] -to [get_clocks{output_clk}]
set_min_delay 3.14 -from [get_clocks{input_clk}] -to [get_clocks{output_clk}]

#
# External Delays
#

#External delay on inputs
set_input_delay -clock input_clk 0.250 [get_ports{in1 in2 in3}]

#External delay on outputs
set_output_delay -clock output_clk 1 [get_ports{out*}]
