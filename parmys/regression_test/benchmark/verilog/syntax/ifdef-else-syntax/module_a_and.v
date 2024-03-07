
module a(clock,
		a_in,
		b_in,
		out);

input	clock;
input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
output [`BITS-1:0]    out;
reg [`BITS-1:0]    out;

always @(posedge clock)
begin
	out <= a_in & b_in;
end

endmodule
