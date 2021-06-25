// DEFINES
`define WIDTH 8         // Bit width 
`define DEPTH 16         // Bit depth


module  pram(
  clock,
  wren1,
  wren2,
  address,
  address2,
  value_in,
  value_in2,
  spram_out,
  dpram_out,
  dpram_out2,
  dpram2_out,
  dpram2_out2
);

// SIGNAL DECLARATIONS
input   clock;
input wren1;
input wren2;

input  [`WIDTH-1:0] value_in;
input  [`WIDTH-1:0] value_in2;

output [`WIDTH-1:0] spram_out;
output [`WIDTH-1:0] dpram_out;
output [`WIDTH-1:0] dpram_out2;
output [`WIDTH-1:0] dpram2_out;
output [`WIDTH-1:0] dpram2_out2;


input [`DEPTH-1:0] address;
input [`DEPTH-1:0] address2;

defparam inst1.ADDR_WIDTH = `DEPTH;
defparam inst1.DATA_WIDTH = `WIDTH;
dual_port_ram inst1(
  .we1(wren1),
  .we2(wren2),
  .clk(clock),
  .data1(value_in),
  .data2(value_in2),
  .out1(dpram_out),
  .out2(dpram_out2),
  .addr1(address),
  .addr2(address2)
);

defparam inst2.ADDR_WIDTH = `DEPTH;
defparam inst2.DATA_WIDTH = `WIDTH;
dual_port_ram inst2(
  .we1(wren1),
  .we2(1'b0),
  .clk(clock),
  .data1(value_in),
  .data2(value_in2),
  .out1(dpram2_out),
  .out2(dpram2_out2),
  .addr1(address),
  .addr2(address2)
);

defparam inst3.ADDR_WIDTH = `DEPTH;
defparam inst3.DATA_WIDTH = `WIDTH;
single_port_ram inst3(
  .we(wren1),
  .clk(clock),
  .data(value_in),
  .out(spram_out),
  .addr(address)
);

endmodule

/**
 * Copying the modules Single Port RAM and Dual Port RAM from vtr_flow/primitives.v 
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

//dual_port_ram module
module dual_port_ram #(
    parameter ADDR_WIDTH = 1,
    parameter DATA_WIDTH = 1
) (
    input clk,

    input [ADDR_WIDTH-1:0] addr1,
    input [ADDR_WIDTH-1:0] addr2,
    input [DATA_WIDTH-1:0] data1,
    input [DATA_WIDTH-1:0] data2,
    input we1,
    input we2,
    output reg [DATA_WIDTH-1:0] out1,
    output reg [DATA_WIDTH-1:0] out2
);

    localparam MEM_DEPTH = 2 ** ADDR_WIDTH;

    reg [DATA_WIDTH-1:0] Mem[MEM_DEPTH-1:0];

    specify
        (clk*>out1)="";
        (clk*>out2)="";
        $setup(addr1, posedge clk, "");
        $setup(addr2, posedge clk, "");
        $setup(data1, posedge clk, "");
        $setup(data2, posedge clk, "");
        $setup(we1, posedge clk, "");
        $setup(we2, posedge clk, "");
        $hold(posedge clk, addr1, "");
        $hold(posedge clk, addr2, "");
        $hold(posedge clk, data1, "");
        $hold(posedge clk, data2, "");
        $hold(posedge clk, we1, "");
        $hold(posedge clk, we2, "");
    endspecify
   
    always@(posedge clk) begin //Port 1
        if(we1) begin
            Mem[addr1] = data1;
        end
        out1 = Mem[addr1]; //New data read-during write behaviour (blocking assignments)
    end

    always@(posedge clk) begin //Port 2
        if(we2) begin
            Mem[addr2] = data2;
        end
        out2 = Mem[addr2]; //New data read-during write behaviour (blocking assignments)
    end
   
endmodule // dual_port_ram
