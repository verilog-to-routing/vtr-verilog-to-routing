create_clock -period 10.0 counter_1.clk_counter
create_clock -period 5.0 counter_2.clk_counter

set_max_delay 40 -from [get_clocks {counter_1.clk_counter}] -to [get_clocks {counter_2.clk_counter}]
