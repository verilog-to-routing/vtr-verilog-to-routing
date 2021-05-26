module top_module (
	input		    clk,
	input       [2:0] a,
	output reg  [2:0] w,
	output reg  [2:0] q
);

	
	always@(posedge clk)
	begin
		w <= a;
	end
	
	always@(negedge clk)
	begin
		q <= a;
	end


endmodule


/* 
  
  [ODIN_SOFT_REG]
  -V ~/Desktop/yosys+odin/reg/reg.v -o ~/Desktop/yosys+odin/reg/odin_soft_reg.blif -t ~/Desktop/yosys+odin/reg/reg_input -T ~/Desktop/yosys+odin/reg/reg_output
  
  [YOSYS_BLIF_SOFT_REG]
  -b ~/Desktop/yosys+odin/reg/yosys_reg.blif -o ~/Desktop/yosys+odin/reg/yosys+odin_soft_reg.blif --coarsen -t ~/Desktop/yosys+odin/reg/reg_input -T ~/Desktop/yosys+odin/reg/reg_output
  
  [YOSYS_CMD]
  read_verilog ~/Desktop/yosys+odin/reg/reg.v; proc; opt; write_blif -top top_module ~/Desktop/yosys+odin/reg/yosys_reg.blif;
*/
