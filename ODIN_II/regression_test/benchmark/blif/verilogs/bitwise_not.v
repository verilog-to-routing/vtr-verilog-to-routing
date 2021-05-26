module top_module (
	input   [3:0] x_top, 
	output  [3:0] q_top
);

	assign q_top = ~x_top;

endmodule


/* 
  [ODIN_HARD bitwise_not]
  -V ~/Desktop/yosys+odin/bitwise_not/bitwise_not.v -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/bitwise_not/odin_hard_bitwise_not.blif -t ~/Desktop/yosys+odin/bitwise_not/bitwise_not_input -T ~/Desktop/yosys+odin/bitwise_not/bitwise_not_output
  
  [YOSYS_BLIF_HARD_bitwise_not]
  -b ~/Desktop/yosys+odin/bitwise_not/yosys_bitwise_not.blif -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/bitwise_not/yosys+odin_hard_bitwise_not.blif --coarsen -t ~/Desktop/yosys+odin/bitwise_not/bitwise_not_input -T ~/Desktop/yosys+odin/bitwise_not/bitwise_not_output
  
  [ODIN_SOFT_bitwise_not]
  -V ~/Desktop/yosys+odin/bitwise_not/bitwise_not.v -o ~/Desktop/yosys+odin/bitwise_not/odin_soft_bitwise_not.blif -t ~/Desktop/yosys+odin/bitwise_not/bitwise_not_input -T ~/Desktop/yosys+odin/bitwise_not/bitwise_not_output
  
  [YOSYS_BLIF_SOFT_bitwise_not]
  -b ~/Desktop/yosys+odin/bitwise_not/yosys_bitwise_not.blif -o ~/Desktop/yosys+odin/bitwise_not/yosys+odin_soft_bitwise_not.blif --coarsen -t ~/Desktop/yosys+odin/bitwise_not/bitwise_not_input -T ~/Desktop/yosys+odin/bitwise_not/bitwise_not_output
  
  [YOSYS_CMD]
  read_verilog ~/Desktop/yosys+odin/bitwise_not/bitwise_not.v; proc; opt; write_blif -top top_module ~/Desktop/yosys+odin/bitwise_not/yosys_bitwise_not.blif;
*/
