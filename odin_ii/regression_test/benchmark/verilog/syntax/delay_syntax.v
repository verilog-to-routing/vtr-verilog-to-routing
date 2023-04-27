`timescale 1ns/10ps


// This one is not working correctly under Quartus simulation

module delay_syntax (clock,a,b,c,d);

input clock;
input a;
output b;
output c;
output d;

reg x;
reg y;
reg delayed;

assign b = x;
assign c = y;
assign #10 d = delayed;

always @ (posedge clock)
	begin
		x <= a;
		y <= x;
		delayed <= x;
	end
	
endmodule