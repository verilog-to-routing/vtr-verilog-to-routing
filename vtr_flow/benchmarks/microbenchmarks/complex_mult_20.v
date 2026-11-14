// Complex multiplier (a_re + j*a_im) * (b_re + j*b_im) with 20-bit unsigned parts.
// Uses four DSPs; p_re is two's complement since a_re*b_re - a_im*b_im can be negative.
module complex_mult_20 (
    input clk,
    input [19:0] a_re,
    input [19:0] a_im,
    input [19:0] b_re,
    input [19:0] b_im,
    output reg [40:0] p_re,
    output reg [40:0] p_im
);
    wire [39:0] re_re = a_re * b_re;
    wire [39:0] im_im = a_im * b_im;
    wire [39:0] re_im = a_re * b_im;
    wire [39:0] im_re = a_im * b_re;

    always @(posedge clk) begin
        p_re <= {1'b0, re_re} - {1'b0, im_im};
        p_im <= {1'b0, re_im} + {1'b0, im_re};
    end
endmodule
