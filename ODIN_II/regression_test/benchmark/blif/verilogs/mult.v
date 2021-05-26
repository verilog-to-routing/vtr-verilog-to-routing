module top_module (
	input   [15:0] x, 
	input   [15:0] y,
	output  [15:0] q
);

	assign q = x * y;

endmodule


/* 
  [ODIN_HARD_MULT]
  -V ~/Desktop/yosys+odin/mult/mult.v -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/mult/odin_hard_mult.blif -t ~/Desktop/yosys+odin/mult/mult_input -T ~/Desktop/yosys+odin/mult/mult_output
  
  [YOSYS_BLIF_HARD_MULT]
  -b ~/Desktop/yosys+odin/mult/yosys_mult.blif -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/mult/yosys+odin_hard_mult.blif -t ~/Desktop/yosys+odin/mult/mult_input -T ~/Desktop/yosys+odin/mult/mult_output
  
  [ODIN_SOFT_MULT]
  -V ~/Desktop/yosys+odin/mult/mult.v -o ~/Desktop/yosys+odin/mult/odin_soft_mult.blif -t ~/Desktop/yosys+odin/mult/mult_input -T ~/Desktop/yosys+odin/mult/mult_output
  
  [YOSYS_BLIF_SOFT_MULT]
  -b ~/Desktop/yosys+odin/mult/yosys_mult.blif -o ~/Desktop/yosys+odin/mult/yosys+odin_soft_mult.blif -t ~/Desktop/yosys+odin/mult/mult_input -T ~/Desktop/yosys+odin/mult/mult_output
  
  [YOSYS_CMD]
  read_verilog ~/Desktop/yosys+odin/mult/mult.v; proc; opt; write_blif ~/Desktop/yosys+odin/mult/yosys_mult.blif;
*/
