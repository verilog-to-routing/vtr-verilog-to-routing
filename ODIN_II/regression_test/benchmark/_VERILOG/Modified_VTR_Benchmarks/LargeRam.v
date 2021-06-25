// DEFINES
`define WIDTH 2        // Bit width 
`define DEPTH 33         // Bit depth

module  pram(
  clock,
  wren1,
  address,
  value_in,
  spram_out,
);

// SIGNAL DECLARATIONS
input   clock;

input wren1;

input  [`WIDTH-1:0] value_in;

output [`WIDTH-1:0] spram_out;


input [`DEPTH-1:0] address;

defparam inst1.ADDR_WIDTH = `DEPTH;
defparam inst1.DATA_WIDTH = `WIDTH;
single_port_ram inst1(
  .we(wren1),
  .clk(clock),
  .data(value_in),
  .out(spram_out),
  .addr(address)
);

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