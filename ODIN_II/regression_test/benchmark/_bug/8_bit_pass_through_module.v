module eight_bit_for_pass_through(a, c, clk, rst);
input [7:0] a;
input clk, rst;
output [7:0] c;
	pass_through pass_wire_0(a[0], c[0], clk, rst);
	pass_through pass_wire_1(a[1], c[1], clk, rst);
	pass_through pass_wire_2(a[2], c[2], clk, rst);
	pass_through pass_wire_3(a[3], c[3], clk, rst);
	pass_through pass_wire_4(a[4], c[4], clk, rst);
	pass_through pass_wire_5(a[5], c[5], clk, rst);
	pass_through pass_wire_6(a[6], c[6], clk, rst);
	pass_through pass_wire_7(a[7], c[7], clk, rst);
endmodule

module pass_through(a, c, clk, rst);
input a, clk, rst;
output c;
always @ ( posedge clk )
begin
    case(rst)
        0: c = a;
        1: c = 0;
    endcase
end
endmodule
