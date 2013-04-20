create_clock -period 2 clk     
create_clock -period 3 clk2
set_clock_groups -exclusive -group {clk} -group {clk2}