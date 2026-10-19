// Instantaneous power of a complex baseband sample: i^2 + q^2 with 24-bit components.
// Each squarer drives both operands of its DSP from the same nets.
module iq_power_24 (
    input clk,
    input [23:0] i,
    input [23:0] q,
    output reg [48:0] power
);
    wire [47:0] i_sq = i * i;
    wire [47:0] q_sq = q * q;

    always @(posedge clk)
        power <= i_sq + q_sq;
endmodule
