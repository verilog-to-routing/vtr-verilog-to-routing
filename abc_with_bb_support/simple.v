// Test enable circuitry

module  simple(clock,
                enable,
                value_out
                );

input   clock;
input   enable;
reg temp;
output value_out;

always @(posedge clock)
begin
	if (enable == 1'b1) begin
		temp <= 1'b0;
	end
end

assign value_out = temp;

endmodule

