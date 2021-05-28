module top_module (
	input   [1:0] x_top, 
	input   [1:0] y_top,
	output  [1:0] q_top/*,
	output  [2:0] w*/
);

//	add_module uut (/*1,*/ x_top, y_top, q_top/*, w*/);
	assign q_top = x_top - y_top;
	

endmodule

//module add_module (
	/*input		  clk,*/
//	input   [1:0] x_add, 
//	input   [1:0] y_add,
//	output  [1:0] q_add
/*,
	output  [2:0] w*/
//);

	/*reg [2:0] req_w = 3'b111;*/
//	add_add_module uut (x_add, y_add, q_add);
	/*always@(posedge clk)
	begin
		w <= reg_w;
	end*/
	

//endmodule

/*module add_add_module (
	input   [1:0] x_add2, 
	input   [1:0] y_add2,
	output  [1:0] q_add2
);

	assign q_add2 = x_add2 + y_add2;
	
endmodule
*/

/* 
  [ODIN_HARD ADDER]
  -V ~/Desktop/yosys+odin/sub/sub.v -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/sub/odin_hard_sub.blif -t ~/Desktop/yosys+odin/sub/sub_input -T ~/Desktop/yosys+odin/sub/sub_output
  
  [YOSYS_BLIF_HARD_ADDER]
  -b ~/Desktop/yosys+odin/sub/yosys_sub.blif -a ~/Desktop/yosys+odin/arch/k6_frac_N10_frac_chain_depop50_mem32K_40nm.xml -o ~/Desktop/yosys+odin/sub/yosys+odin_hard_sub.blif --coarsen -t ~/Desktop/yosys+odin/sub/sub_input -T ~/Desktop/yosys+odin/sub/sub_output
  
  [ODIN_SOFT_ADDER]
  -V ~/Desktop/yosys+odin/sub/sub.v -o ~/Desktop/yosys+odin/sub/odin_soft_sub.blif -t ~/Desktop/yosys+odin/sub/sub_input -T ~/Desktop/yosys+odin/sub/sub_output
  
  [YOSYS_BLIF_SOFT_ADDER]
  -b ~/Desktop/yosys+odin/sub/yosys_sub.blif -o ~/Desktop/yosys+odin/sub/yosys+odin_soft_sub.blif --coarsen -t ~/Desktop/yosys+odin/sub/sub_input -T ~/Desktop/yosys+odin/sub/sub_output
  
  [YOSYS_CMD]
  read_verilog ~/Desktop/yosys+odin/sub/sub.v; proc; opt; write_blif -top top_module ~/Desktop/yosys+odin/sub/yosys_sub.blif;
*/
