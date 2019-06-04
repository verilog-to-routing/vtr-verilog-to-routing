module eight_bit_while_pass_through(
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
		0:begin
			i = 0;
			while(i<8)
			begin 
				c[i] = a[i];
				i = i + 1;
			end
		end
		1: c = 0;
	endcase
end
endmodule
