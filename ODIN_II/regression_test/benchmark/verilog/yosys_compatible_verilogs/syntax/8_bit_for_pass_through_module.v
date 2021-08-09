module eight_bit_for_pass_through(a, c, clk, rst);
input [7:0] a;
input clk, rst;
output [7:0] c;
genvar i;
generate
	for(i=0; i<8; i = i+1)
	begin
		pass_through pass_wire(a[i], c[i], clk, rst);
	end	
endgenerate
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
