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

single_port_ram inst1(
  .we(wren1),
  .clk(clock),
  .data(value_in),
  .out(spram_out),
  .addr(address)
);

endmodule

