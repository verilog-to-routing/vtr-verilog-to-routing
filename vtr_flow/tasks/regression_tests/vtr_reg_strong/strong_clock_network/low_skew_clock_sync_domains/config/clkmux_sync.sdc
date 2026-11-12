# clk1 and clk2 both reach the flip-flops through a LUT clock mux, so the
# connections to the flip-flop clock pins carry both clocks. The clocks are
# synchronous, so they should be in one low-skew clock group.
create_clock -period 5.0 -name clk1 [get_ports {clk1}]
create_clock -period 10.0 -name clk2 [get_ports {clk2}]
