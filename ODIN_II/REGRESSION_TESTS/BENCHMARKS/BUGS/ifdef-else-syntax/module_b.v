
module b(clock,
		a_in,
		b_in,
		out);

input	clock;
input [`BITS-1:0] a_in;
input [`BITS-1:0] b_in;
wire [`BITS-1:0] temp;
output [`BITS-1:0] out;
reg [`BITS-1:0] out;

a my_a(clock, a_in, b_in, temp);

always @(posedge clock)
begin
	out <= a_in & temp;
end

endmodule
