module seven_of_eight_bit_for_pass_through(
    a,
    c,
    clk,
    rst
);

input [7:0] a;
input clk, rst;
output [7:0] c;
integer i;
parameter init = 0;

always @ ( posedge clk )
begin
    case(rst)
        0: for(i = init; i<8; i = i + 1)
           begin 
               c[i] = a[i];
           end
        1: c = 0;
    endcase
end
endmodule
