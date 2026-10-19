// Full-width 27x27 unsigned multiplier.
// Both operands come straight from input pads, so their bits reach the DSP from different
// sides of the device and a per-bit router is tempted to split the operand bus muxes.
module mult_27x27 (
    input [26:0] a,
    input [26:0] b,
    output [53:0] p
);
    assign p = a * b;
endmodule
