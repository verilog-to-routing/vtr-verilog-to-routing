# Clock
create_clock -period 2.1 [get_ports clk]

# Apply a 500ps uncertainty to the clock for both setup and hold
set_clock_uncertainty -to [get_clocks {clk}] 0.5

# Constrain all I/Os
set_input_delay -clock clk -max 0 [get_ports {*}]
set_output_delay -clock clk -max 0 [get_ports {*}]
