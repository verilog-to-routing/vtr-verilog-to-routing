# Two logically-exclusive clocks entering the design on separate input ports.
# An internal .names clock mux selects between them; both clock trees are
# physically present in the design simultaneously, so -logically_exclusive
# is the correct group type (vs. -physically_exclusive for a board-level mux).
#
# The two clocks have distinct source ports so -add is not required.
# VPR's clock auto-detection traces through the .names mux back to these
# primary input ports, which are the recognized clock source pins.
create_clock -period 5.0 -name clk_fast [get_ports {clk_fast}]
create_clock -period 10.0 -name clk_slow [get_ports {clk_slow}]
set_clock_groups -logically_exclusive -group {clk_fast} -group {clk_slow}
