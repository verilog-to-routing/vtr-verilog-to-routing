// DEFINES
`define WIDTH 16         // Bit width 
`define DEPTH 8         // Bit depth

module  spram_big(clock,
                reset_n,
                value_out,
                value_in
                );

// SIGNAL DECLARATIONS
input   clock;
input   reset_n;
input [`WIDTH-1:0] value_in;
output [`WIDTH-1:0] value_out;
wire [`WIDTH-1:0] value_out;

reg [`DEPTH-1:0] address_counter;
reg [`WIDTH-1:0] temp;

single_port_ram inst1(
  .we(clock),
  .data(value_in),
  .out(value_out),
  .addr(address_counter));

always @(posedge clock)
begin
	if (reset_n == 1'b1) begin
		address_counter <= 4'b00000000;
	end
	else
	begin
		address_counter <= address_counter + 1;
	end
end

endmodule

