// DEFINES
`define BITS 32         // Bit width of the operands

module macro_in_module_declaration(
        input clk,
        input [`BITS-1:0] in,
        output [`BITS-1:0] out
        );

always @(posedge clk)
begin
    out <= in;
end

endmodule 