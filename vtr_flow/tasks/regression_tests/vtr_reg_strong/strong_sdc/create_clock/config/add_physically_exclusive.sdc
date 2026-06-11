# Two physically-exclusive clocks on the same source port, using -add.
# clk_fast and clk_slow are driven by a board-level mux; only one can
# be active at a time so they are physically exclusive.
# No timing paths between the two clock domains are analyzed.
create_clock -period 5.0 -name clk_fast [get_ports {clk}]
create_clock -period 10.0 -name clk_slow -add [get_ports {clk}]
set_clock_groups -physically_exclusive -group {clk_fast} -group {clk_slow}
