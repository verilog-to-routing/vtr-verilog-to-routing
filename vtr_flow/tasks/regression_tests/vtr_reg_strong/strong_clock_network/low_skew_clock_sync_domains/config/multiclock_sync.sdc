# Two synchronous clocks: transfers between clk and clk2 are analyzed, so both
# clock domains should be in one low-skew clock group.
create_clock -period 10 clk
create_clock -period 10 clk2
