module top_module (
	input 	 rst,
	input   [1:0] x_top, 
	input   [1:0] y_top,
	output  [1:0] q_top/*,
	output  [1:0] w_top*/
);

	wire [1:0] q_temp;

	add_add_module uut (/*1,*/ x_top, y_top, q_temp/*, w*/);
	assign q_top = (rst) ? 2'd0 : q_temp;
	//assign q_top = x_top + y_top;
	//assign w_top = y_top + x_top;

endmodule



module add_add_module (
	input   [1:0] x_add2, 
	input   [1:0] y_add2,
	output  [1:0] q_add2
);

	assign q_add2 = x_add2 + y_add2;
	
endmodule


/* 
  [ODIN_HARD ADDER]
  -V ~/Desktop/yosys+odin/adder/adder.v -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/adder/odin_hard_adder.blif -t ~/Desktop/yosys+odin/adder/adder_input -T ~/Desktop/yosys+odin/adder/adder_output
  
  [YOSYS_BLIF_HARD_ADDER]
  -b ~/Desktop/yosys+odin/adder/yosys_adder.blif -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/adder/yosys+odin_hard_adder.blif --coarsen -t ~/Desktop/yosys+odin/adder/adder_input -T ~/Desktop/yosys+odin/adder/adder_output
  
  [ODIN_SOFT_ADDER]
  -V ~/Desktop/yosys+odin/adder/adder.v -o ~/Desktop/yosys+odin/adder/odin_soft_adder.blif -t ~/Desktop/yosys+odin/adder/adder_input -T ~/Desktop/yosys+odin/adder/adder_output
  
  [YOSYS_BLIF_SOFT_ADDER]
  -b ~/Desktop/yosys+odin/adder/yosys_adder.blif -o ~/Desktop/yosys+odin/adder/yosys+odin_soft_adder.blif --coarsen -t ~/Desktop/yosys+odin/adder/adder_input -T ~/Desktop/yosys+odin/adder/adder_output
  
  [YOSYS_CMD]
  read_verilog ~/Desktop/yosys+odin/adder/adder.v; proc; opt; write_blif -top top_module ~/Desktop/yosys+odin/adder/yosys_adder.blif;
*/
