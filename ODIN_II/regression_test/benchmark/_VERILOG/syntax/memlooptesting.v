

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

/**
 * Copying the modules Single Port RAM from vtr_flow/primitives.v 
 * to correct the hard block inferrence by Yosys 
*/
//single_port_ram module
module single_port_ram #(
    parameter ADDR_WIDTH = 1,
    parameter DATA_WIDTH = 1
) (
    input clk,
    input [ADDR_WIDTH-1:0] addr,
    input [DATA_WIDTH-1:0] data,
    input we,
    output reg [DATA_WIDTH-1:0] out
);

    localparam MEM_DEPTH = 2 ** ADDR_WIDTH;

    reg [DATA_WIDTH-1:0] Mem[MEM_DEPTH-1:0];

    specify
        (clk*>out)="";
        $setup(addr, posedge clk, "");
        $setup(data, posedge clk, "");
        $setup(we, posedge clk, "");
        $hold(posedge clk, addr, "");
        $hold(posedge clk, data, "");
        $hold(posedge clk, we, "");
    endspecify
   
    always@(posedge clk) begin
        if(we) begin
            Mem[addr] = data;
        end
    	out = Mem[addr]; //New data read-during write behaviour (blocking assignments)
    end
   
endmodule // single_port_RAM