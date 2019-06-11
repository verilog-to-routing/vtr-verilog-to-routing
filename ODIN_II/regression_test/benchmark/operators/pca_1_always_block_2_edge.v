module simple_op(reset,
	set,
	a,
	q
);

input reset;
input set;
input a;

output q;
reg q;

always @(*)
begin

	if (reset)
		assign q = 1'b0;

	else
	if (set)

		assign q = 1'b1;

	else
	begin
		deassign q;

	end

end

endmodule
