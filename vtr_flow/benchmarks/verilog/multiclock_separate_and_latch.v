module multiclock_separate_and_latch(
    clocks,
	a_in,
	b_in,
	out
);

    // SIGNAL DECLARATIONS
    input	[1:0]   clocks;
    input	[1:0]   a_in;
    input	[1:0]   b_in;

    output	[1:0]   out;

    and_latch and_latch_zero(
        clocks[0],
        a_in[0],
        b_in[0],
        out[0]
    );

    and_latch and_latch_one(
        clocks[1],
        a_in[1],
        b_in[1],
        out[1]
    );

endmodule

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