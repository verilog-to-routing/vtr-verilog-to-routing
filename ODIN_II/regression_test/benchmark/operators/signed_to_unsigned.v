module mymod
(
    clk, out1, out2, out3, out4
);
    parameter a = -5;

    input clk;
    output reg [31:0] out1, out2, out3, out4;

    wire [15:0] wx;
    wire [31:0] wy1, wy2;

    assign wx = 4'she * 4'sb0100;

    assign wy1 = -5 >>> 4;
    assign wy2 = a >>> 4;
   
    always @ (posedge clk)
    begin
        out1 <= 4'she * 4'sb0100;
        out2 <= wx;
        out3 <= wy1;
        out4 <= wy2;
    end

endmodule