module bm_and_log(
    clock_reader_head,
    clock_writer_head,
	in,
	out,
);

	//INPUT
	input	clock_reader_head;
	input	clock_writer_head;
	input	in;

	//OUTPUT
	output	out;

	reg	[1:0]	reader_head;
	reg	[1:0]	writer_head;
	reg [3:0]	register;

	always @(posedge clock_reader_head)
	begin

		case (reader_head)	
			0: register[0] <= in;
			1: register[1] <= in;
			2: register[2] <= in;
			3: register[3] <= in;
		endcase

		reader_head <= reader_head +1;
	end

	always @(posedge clock_writer_head)
	begin

		case (reader_head)	
			0: out <= register[0];
			1: out <= register[1];
			2: out <= register[2];
			3: out <= register[3];
		endcase

		writer_head <= writer_head +1;
	end

endmodule
