module top_module (
	input   [3:0] x_top, 
	output  [3:0] q_top
);

	assign q_top = !x_top;

endmodule


/* 
  [ODIN_HARD NOT]
  -V ~/Desktop/yosys+odin/not/not.v -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/not/odin_hard_not.blif -t ~/Desktop/yosys+odin/not/not_input -T ~/Desktop/yosys+odin/not/not_output
  
  [YOSYS_BLIF_HARD_NOT]
  -b ~/Desktop/yosys+odin/not/yosys_not.blif -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/not/yosys+odin_hard_not.blif --coarsen -t ~/Desktop/yosys+odin/not/not_input -T ~/Desktop/yosys+odin/not/not_output
  
  [ODIN_SOFT_NOT]
  -V ~/Desktop/yosys+odin/not/not.v -o ~/Desktop/yosys+odin/not/odin_soft_not.blif -t ~/Desktop/yosys+odin/not/not_input -T ~/Desktop/yosys+odin/not/not_output
  
  [YOSYS_BLIF_SOFT_NOT]
  -b ~/Desktop/yosys+odin/not/yosys_not.blif -o ~/Desktop/yosys+odin/not/yosys+odin_soft_not.blif --coarsen -t ~/Desktop/yosys+odin/not/not_input -T ~/Desktop/yosys+odin/not/not_output
  
  [YOSYS_CMD]
  read_verilog ~/Desktop/yosys+odin/not/not.v; proc; opt; write_blif -top top_module ~/Desktop/yosys+odin/not/yosys_not.blif;
*/
