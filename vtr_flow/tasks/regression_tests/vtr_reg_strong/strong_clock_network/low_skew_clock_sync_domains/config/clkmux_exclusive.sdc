# Same LUT clock mux as clkmux_sync, but the two clocks are logically
# exclusive, so they should be in separate low-skew clock groups.
create_clock -period 5.0 -name clk1 [get_ports {clk1}]
create_clock -period 10.0 -name clk2 [get_ports {clk2}]
set_clock_groups -logically_exclusive -group {clk1} -group {clk2}
