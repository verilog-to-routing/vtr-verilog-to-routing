// DEFINES
`define BITS 32         // Bit width of the operands

module module_in_out_declaration(
        input clk,
        input [`BITS-1:0] in,
        output [`BITS-1:0] out
        );
  
always @(posedge clk)
begin
    out <= in;
end
  
endmodule