module simple_op(reset, 
	set1, 
	set2, 
	set, 
	a, 
	q
);

	input reset;
	input set;
	input set1;
	input set2;
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
	if (set1)

		assign q = 1'b0;

	else
	if (set2)

		assign q = 1'b1;

	else
	begin

		deassign q;

	end

end

endmodule
