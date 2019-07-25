module asr_16_7(
    a,
    c,
    clk,
    rst
);

input [15:0] a;
input clk, rst;
output [15:0] c;

always @ ( posedge clk )
begin
    case(rst)
        0: c = a >>> 7;
        1: c = 0;
    endcase
end
endmodule
