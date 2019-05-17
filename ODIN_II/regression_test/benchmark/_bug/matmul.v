`define WORD_SIZE 16

`define ROW_SIZE 2
`define COL_SIZE 2

`define ADDR_SIZE (`ROW_SIZE*`COL_SIZE)
`define RAM_DEPTH (2**`ADDR_SIZE)

module matrix_multiplication (
	input clk, 
	input reset, 
	input we, 
	input start, 
	input [`WORD_SIZE-1:0] data_in, 
	output [`WORD_SIZE-1:0] data_out
); 

	wire [`WORD_SIZE-1:0] mat_A;  
	reg [`ADDR_SIZE-1:0] addr_in;  

	//Local (distributed) memory
	reg [`WORD_SIZE-1:0] matrixA[`ROW_SIZE-1:0][`COL_SIZE-1:0];

	// BRAM matrix A  
	single_port_ram matrix_A_u (
		.clk	( clk ),
		.we		( we ),
		.data	( data_in ),
		.out	( mat_A ),
		.addr	( addr_in )
	);

	always @(posedge clk or posedge reset)  
	begin  
		if(reset && !start) begin  
			addr_in <= 0;  
		end  
		else begin  
			if(addr_in < (`RAM_DEPTH-1) )
				addr_in = addr_in + 1;  

			matrixA[ addr_in / `COL_SIZE ][ addr_in - ((addr_in / `COL_SIZE)*`COL_SIZE) ] = mat_A ;  
			data_out = mat_A;
		end  
	end  
			 
 endmodule
