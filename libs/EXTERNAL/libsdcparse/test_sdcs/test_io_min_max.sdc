#I/O Delay
set_input_delay -clock input_clk -max -min 0.5 [get_ports{in1 in2 in3}]
set_output_delay -clock output_clk -max -min 1 [get_ports{out*}]
set_input_delay -clock input_clk 0.5 [get_ports{in1 in2 in3}]
set_output_delay -clock output_clk 1 [get_ports{out*}]
