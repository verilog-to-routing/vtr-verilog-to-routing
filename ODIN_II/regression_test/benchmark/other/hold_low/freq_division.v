module frequency_divide( 
	clk,
	reset,
	base_clk,
	clk_div2,
	clk_div4,
	clk_div8
);
	/* input declaration */
	input clk; 
	input reset;
	/* output declaration */
	output base_clk;
	output clk_div2;
	output clk_div4;
	output clk_div8;

	/*  intermediate declaration */
	reg [2:0] counter;

	/* intermediate to output assignments */
	assign base_clk = clk;
	assign clk_div2 = counter[0];
	assign clk_div4 = counter[1];
	assign clk_div8 = counter[2];

	always @(posedge clk)
	begin
		if (reset)
		begin
			counter <= 3'b000;
		end
		else
		begin
			case (counter)
				3'b000:  begin	counter <= 3'b001;	end
				3'b001:	 begin	counter <= 3'b010;	end
				3'b010:	 begin	counter <= 3'b011;	end
				3'b011:	 begin	counter <= 3'b100;	end
				3'b100:	 begin	counter <= 3'b101;	end
				3'b101:	 begin	counter <= 3'b110;	end
				3'b110:	 begin	counter <= 3'b111;	end
				default: begin	counter <= 3'b000;	end
			endcase
		end
	end


endmodule
