

module memory_loop(d_addr,d_loadresult,clk,wren,d1, do_something);


input do_something;
input wren;
input clk;
input [31:0] d1;
input [3:0] d_addr;
output [31:0] d_loadresult;


reg [31:0] d_loadresult;

wire [31:0] loaded_data;

single_port_ram smem_replace(
  .clk (clk),
  .we(wren),
  .data(d1),
  .out(loaded_data),
  .addr(d_addr));

//assign d_loadresult = loaded_data;			// trivial circuit in/out works (minus outputs "not beign driven")

always @(posedge clk)			//procedural block dependant on memory.out
begin
    case (do_something)
	
	1'b0:
		begin

			 d_loadresult <= loaded_data;			//this line creates "combinational loops"
		end
	
	1'b1: 
		begin

			d_loadresult <= ~loaded_data;
		end
	
	endcase
end
	
endmodule