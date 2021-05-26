module top_module (
	input 	[10:0] rst,/*
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
  [ODIN_HARD reduce_bool]
  -V ~/Desktop/yosys+odin/reduce_bool/reduce_bool.v -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/reduce_bool/odin_hard_reduce_bool.blif -t ~/Desktop/yosys+odin/reduce_bool/reduce_bool_input -T ~/Desktop/yosys+odin/reduce_bool/reduce_bool_output
  
  [YOSYS_BLIF_HARD_reduce_bool]
  -b ~/Desktop/yosys+odin/reduce_bool/yosys_reduce_bool.blif -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/reduce_bool/yosys+odin_hard_reduce_bool.blif --coarsen -t ~/Desktop/yosys+odin/reduce_bool/reduce_bool_input -T ~/Desktop/yosys+odin/reduce_bool/reduce_bool_output
  
  [ODIN_SOFT_reduce_bool]
  -V ~/Desktop/yosys+odin/reduce_bool/reduce_bool.v -o ~/Desktop/yosys+odin/reduce_bool/odin_soft_reduce_bool.blif -t ~/Desktop/yosys+odin/reduce_bool/reduce_bool_input -T ~/Desktop/yosys+odin/reduce_bool/reduce_bool_output
  
  [YOSYS_BLIF_SOFT_reduce_bool]
  -b ~/Desktop/yosys+odin/reduce_bool/yosys_reduce_bool.blif -o ~/Desktop/yosys+odin/reduce_bool/yosys+odin_soft_reduce_bool.blif --coarsen -t ~/Desktop/yosys+odin/reduce_bool/reduce_bool_input -T ~/Desktop/yosys+odin/reduce_bool/reduce_bool_output
  
  [YOSYS_CMD]
  read_verilog ~/Desktop/yosys+odin/reduce_bool/reduce_bool.v; proc; opt; write_blif -top top_module ~/Desktop/yosys+odin/reduce_bool/yosys_reduce_bool.blif;
*/
