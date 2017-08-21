create_clock -period 0.5 -name VIRTUAL_INPUT_CLOCK
create_clock -period 0.4 -name VIRTUAL_OUTPUT_CLOCK
create_clock -period 0.7 clk

set_input_delay -clock VIRTUAL_INPUT_CLOCK -max 0.1 [get_ports{*}]
set_output_delay -clock VIRTUAL_OUTPUT_CLOCK -max 0.2 [get_ports{*}]

set_clock_groups -exclusive -group [get_clocks {VIRTUAL_INPUT_CLOCK}] -group [get_clocks {VIRTUAL_OUTPUT_CLOCK}]

set_multicycle_path -from [get_clocks {VIRTUAL_INPUT_CLOCK}] -to [get_clocks {clk}] -setup 2
set_multicycle_path -from [get_clocks {VIRTUAL_INPUT_CLOCK}] 2
set_multicycle_path -from [get_clocks {VIRTUAL_INPUT_CLOCK}] -to [get_clocks {clk}] -hold 1
set_multicycle_path -from [get_clocks {VIRTUAL_INPUT_CLOCK}] -hold 1
set_multicycle_path -to [get_clocks {VIRTUAL_INPUT_CLOCK}] -hold 1

set_clock_uncertainty 0.05
set_clock_uncertainty -hold 0.05 -from [get_clocks {clk}]
set_clock_uncertainty -setup 0.05 -from [get_clocks {clk}] -to [get_clocks {VIRTUAL_OUTPUT_CLOCK}]
set_clock_uncertainty -hold -setup 0.05 -from [get_clocks {clk}] -to [get_clocks {VIRTUAL_OUTPUT_CLOCK}]
set_clock_uncertainty 5 -from [get_clocks {clk}] -to [get_clocks {VIRTUAL_OUTPUT_CLOCK}]

set_clock_latency -source 1 [get_clocks {clk}]

