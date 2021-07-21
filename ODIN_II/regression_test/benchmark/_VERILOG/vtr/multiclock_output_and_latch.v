module multiclock_output_and_latch(
    clock0,
    clock1,
	a_in,
	b_in,
	out
);

    // SIGNAL DECLARATIONS
    input   clock0;
    input	clock1;
    input	[1:0]   a_in;
    input	[1:0]   b_in;

    output  out;

    reg wire_selector;
    reg	[1:0]   temp;

    assign out = (wire_selector == 0)? temp[0]: temp[1];

    and_latch and_latch_zero(
        clock0,
        a_in[0],
        b_in[0],
        temp[0]
    );

    and_latch and_latch_one(
        clock0,
        a_in[1],
        b_in[1],
        temp[1]
    );

    always @(posedge clock1)
    begin
        wire_selector <= wire_selector + 1;
    end

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