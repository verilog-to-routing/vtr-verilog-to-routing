module top_module (
	input		    clk,
	input			rst,
	input       [1:0]a,b,
	output reg  [1:0] w
);

	
	always@(posedge rst, posedge clk)
	begin
		if (rst)
			w <= b;
		else
			w <= a;
	end


endmodule


/* 
  
  [ODIN_SOFT_REG]
  -V ~/Desktop/yosys+odin/dffsr/dffsr.v -o ~/Desktop/yosys+odin/dffsr/odin_soft_dffsr.blif -t ~/Desktop/yosys+odin/dffsr/dffsr_input -T ~/Desktop/yosys+odin/dffsr/dffsr_output
  
  [YOSYS_BLIF_SOFT_REG]
  -b ~/Desktop/yosys+odin/dffsr/yosys_dffsr.blif -o ~/Desktop/yosys+odin/dffsr/yosys+odin_soft_dffsr.blif --coarsen -t ~/Desktop/yosys+odin/dffsr/dffsr_input -T ~/Desktop/yosys+odin/dffsr/dffsr_output
  
  [YOSYS_CMD]
  read_verilog ~/Desktop/yosys+odin/dffsr/dffsr.v; proc; opt; write_blif -top top_module ~/Desktop/yosys+odin/dffsr/yosys_dffsr.blif;
*/
