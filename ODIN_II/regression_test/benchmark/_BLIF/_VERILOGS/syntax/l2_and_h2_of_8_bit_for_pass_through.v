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

always @ ( posedge clk )
begin
    case(rst)
        0: 
        begin
            for(i = 0; i<1 || i<2; i = i + 1)
            begin 
                c[i] = a[i];
            end
            for(i = 6; !(i>7); i = i + 1)
            begin
                c[i] = a[i];
            end
        end
        1: c = 0;
    endcase
end
endmodule
