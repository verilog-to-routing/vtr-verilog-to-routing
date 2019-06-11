module simple_op(reset, 
	set, 
	a, 
	q, 
	q1
);
	input reset;
	input set;
	input a;
	output q1;
	output q;

	reg q;
	reg q1;

always @(*)
begin

	if (reset)
		assign q = 1'b0;

	else
		deassign q;

end


always @(*)
begin

	if (set)
		assign q1 = 1'b1;

	else
		deassign q1;


end

endmodule
