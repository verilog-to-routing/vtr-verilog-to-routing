// DEFINES
`define WIDTH 8         // Bit width 
`define DEPTH 3         // Bit depth

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

single_port_ram inst1(
  .we(wren1),
  .clk(clock),
  .data(value_in),
  .out(spram_out),
  .addr(address)
);

endmodule

