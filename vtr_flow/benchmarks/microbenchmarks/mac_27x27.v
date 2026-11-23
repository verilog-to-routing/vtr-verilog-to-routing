// Multiply-accumulate unit (the core of a dot-product engine): acc += a * b.
// clr synchronously clears the accumulator.
module mac_27x27 (
    input clk,
    input clr,
    input [26:0] a,
    input [26:0] b,
    output reg [63:0] acc
);
    wire [53:0] prod = a * b;

    always @(posedge clk) begin
        if (clr)
            acc <= 64'd0;
        else
            acc <= acc + prod;
    end
endmodule
