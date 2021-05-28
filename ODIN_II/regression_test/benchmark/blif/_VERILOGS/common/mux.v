module top_module (
	input 	 rst,/*
	input   [1:0] x_top, 
	input   [1:0] y_top,*/
	output  [1:0] q_top/*,
	output  [2:0] w*/
);

	wire [1:0] q_temp;
	assign q_temp = 2'b10;

	assign q_top = (rst) ? 2'd0 : q_temp;
	

endmodule

/* 
  [ODIN_HARD mux]
  -V ~/Desktop/yosys+odin/mux/mux.v -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/mux/odin_hard_mux.blif -t ~/Desktop/yosys+odin/mux/mux_input -T ~/Desktop/yosys+odin/mux/mux_output
  
  [YOSYS_BLIF_HARD_mux]
  -b ~/Desktop/yosys+odin/mux/yosys_mux.blif -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/mux/yosys+odin_hard_mux.blif --coarsen -t ~/Desktop/yosys+odin/mux/mux_input -T ~/Desktop/yosys+odin/mux/mux_output
  
  [ODIN_SOFT_mux]
  -V ~/Desktop/yosys+odin/mux/mux.v -o ~/Desktop/yosys+odin/mux/odin_soft_mux.blif -t ~/Desktop/yosys+odin/mux/mux_input -T ~/Desktop/yosys+odin/mux/mux_output
  
  [YOSYS_BLIF_SOFT_mux]
  -b ~/Desktop/yosys+odin/mux/yosys_mux.blif -o ~/Desktop/yosys+odin/mux/yosys+odin_soft_mux.blif --coarsen -t ~/Desktop/yosys+odin/mux/mux_input -T ~/Desktop/yosys+odin/mux/mux_output
  
  [YOSYS_CMD]
  read_verilog ~/Desktop/yosys+odin/mux/mux.v; proc; opt; write_blif -top top_module ~/Desktop/yosys+odin/mux/yosys_mux.blif;
*/
