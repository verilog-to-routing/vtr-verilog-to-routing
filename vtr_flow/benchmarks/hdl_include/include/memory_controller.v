/*
 * This Verilog file provides the memory_controller description 
 * for ch_intrinsic_modified.v located at: 
 *      vtr_flow/benchmarks/hdl_include/ch_intrinsic_modified.v
*/
module memory_controller
(
	clk,
	memory_controller_address,
	memory_controller_write_enable,
	memory_controller_in,
	memory_controller_out
);
input clk;
input [`MEMORY_CONTROLLER_ADDR_SIZE-1:0] memory_controller_address;
input memory_controller_write_enable;
input [`MEMORY_CONTROLLER_DATA_SIZE-1:0] memory_controller_in;
output  [`MEMORY_CONTROLLER_DATA_SIZE-1:0] memory_controller_out;
reg [`MEMORY_CONTROLLER_DATA_SIZE-1:0] memory_controller_out;


reg [4:0] str_address;
reg str_write_enable;
reg [7:0] str_in;
wire [7:0] str_out;

single_port_ram _str (
	.clk( clk ),
	.addr( str_address ),
	.we( str_write_enable ),
	.data( str_in ),
	.out( str_out )
);


wire  tag;

//must use all wires inside module.....
assign tag = |memory_controller_address & |memory_controller_address & | memory_controller_in;
reg [`MEMORY_CONTROLLER_TAG_SIZE-1:0] prevTag;
always @(posedge clk)
	prevTag <= tag;
always @( tag or memory_controller_address or memory_controller_write_enable or memory_controller_in)
begin

case(tag)

	1'b0:
	begin
		str_address = memory_controller_address[5-1+0:0];
		str_write_enable = memory_controller_write_enable;
		str_in[8-1:0] = memory_controller_in[8-1:0];
	end
endcase

case(prevTag)

	1'b0:
		memory_controller_out = str_out;
endcase
end

endmodule 

