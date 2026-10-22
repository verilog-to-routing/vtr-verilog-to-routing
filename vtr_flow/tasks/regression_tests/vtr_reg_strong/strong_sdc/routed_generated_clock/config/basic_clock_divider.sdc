create_clock -period 6.0 -name master_clk clk
create_generated_clock -source clk -divide_by 2 [get_pins {div_clk.Q[0]}] -name div_clk

set_input_delay -clock master_clk 1.0 [get_ports in]
set_output_delay -clock div_clk 1.0 [get_ports out]
