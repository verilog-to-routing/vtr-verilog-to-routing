module multiclock_reader_writer(
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
	output reg out;

	reg	[2:0]	reader_head;
	reg	[2:0]	writer_head;
	reg [7:0]	register;

	always @(posedge clock_reader_head)
	begin

		case (reader_head)	
			0: register[0] <= in;
			1: register[1] <= in;
			2: register[2] <= in;
			3: register[3] <= in;
			4: register[4] <= in;
			5: register[5] <= in;
			6: register[6] <= in;
			7: register[7] <= in;
		endcase

		if(reader_head == 7)
			reader_head <= 0;
		else
			reader_head <= reader_head +1;

	end

	always @(posedge clock_writer_head)
	begin

		case (writer_head)	
			0: out <= register[0];
			1: out <= register[1];
			2: out <= register[2];
			3: out <= register[3];
			4: out <= register[0];
			5: out <= register[1];
			6: out <= register[2];
			7: out <= register[3];
		endcase

		if(writer_head == 7)
			writer_head <= 0;
		else
			writer_head <= writer_head +1;
	end

endmodule
