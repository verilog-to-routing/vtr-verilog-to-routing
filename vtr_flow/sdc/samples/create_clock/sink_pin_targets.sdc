# Create clocks on all pins with "clk" in their names.
create_clock -period 10.0 [get_pins *clk*]