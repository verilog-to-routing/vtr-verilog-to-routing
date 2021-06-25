module and_latch(
    clock,
	a_in,
	b_in,
	out
);

    // SIGNAL DECLARATIONS
    input	clock;
    input	a_in;
    input	b_in;

    output	out;

    // ASSIGN STATEMENTS
    always @(posedge clock)
    begin
        out <= a_in & b_in;
    end

endmodule