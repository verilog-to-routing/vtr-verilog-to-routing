// DEFINES
`define WIDTH 8         // Bit width 
`define DEPTH 4         // Bit depth

module  dpram(clock,
		wren1,
		wren2,
                reset_n,
                value_out,
                value_out2,
                value_in,
                value_in2
                );

// SIGNAL DECLARATIONS
input   clock;
input   reset_n;

input wren1;
input wren2;

input  [`WIDTH-1:0] value_in;
input  [`WIDTH-1:0] value_in2;

output [`WIDTH-1:0] value_out;
output [`WIDTH-1:0] value_out2;


wire [`WIDTH-1:0] value_out;
wire [`WIDTH-1:0] value_out2;


reg [`DEPTH-1:0] address_counter;
reg [`DEPTH-1:0] address_counter2;
reg [`WIDTH-1:0] temp;
reg [`WIDTH-1:0] temp2;

dual_port_ram inst1(
  .we1(wren1),
  .we2(wren2),
  .clk(clock),
  .data1(value_in),
  .data2(value_in2),
  .out1(value_out),
  .out2(value_out2),
  .addr1(address_counter),
  .addr2(address_counter2));

always @(posedge clock)
begin
	if (reset_n == 1'b1) begin
		address_counter <= 4'b0000;
		address_counter2 <= 4'b1111;
	end
	else
	begin
		address_counter <= address_counter + 1;
		address_counter2 <= address_counter2 - 1;
	end
end

endmodule

