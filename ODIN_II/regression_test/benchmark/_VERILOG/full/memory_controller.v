// A benchmark to test hard block single port memories. 
`define MEMORY_CONTROLLER_ADDR_SIZE 4
`define MEMORY_CONTROLLER_DATA_SIZE 4

module memory_controller
(
	clk,
	addr1,
	addr2,
	we1,
	we2,
	data1,
	data2,
	sp_out, 
	dp_out1,
	dp_out2
);

	input clk;

	input we1, we2; 

	input  [`MEMORY_CONTROLLER_ADDR_SIZE-1:0] addr1,addr2; 

	input  [`MEMORY_CONTROLLER_DATA_SIZE-1:0] data1,data2;

	output [`MEMORY_CONTROLLER_DATA_SIZE-1:0] sp_out,dp_out1,dp_out2;
	reg    [`MEMORY_CONTROLLER_DATA_SIZE-1:0] sp_out,dp_out1,dp_out2;	

	
	wire [`MEMORY_CONTROLLER_DATA_SIZE-1:0] spi_out,dpi_out1,dpi_out2;

	single_port_ram sp_ram (		
		.addr (addr1),
		.data (data1),
		.we   (we1),	
		.clk  (clk),	
		.out  (spi_out)
	);

	dual_port_ram dp_ram (
                .addr1 (addr1),
		.addr2 (addr2),
                .data1 (data1),
		.data2 (data2),
        	.we1 (we1),
		.we2 (we2),
	        .clk (clk),
        	.out1 (dpi_out1),
		.out2 (dpi_out2)
        );

	always @ (posedge clk)
	begin 
		sp_out <= spi_out; 
		dp_out1 <= dpi_out1; 
		dp_out2 <= dpi_out2; 
	end
endmodule 
