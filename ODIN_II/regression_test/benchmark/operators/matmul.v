`define WORD_SIZE 2

`define ROW_SIZE 2
`define COL_SIZE 2

`define ADDR_SIZE (`ROW_SIZE*`COL_SIZE)
`define RAM_DEPTH (2**`ADDR_SIZE)

module matrix_multiplication (
	input clk, 
	input rst,
	output [`WORD_SIZE-1:0] data_out,
	output direction,
	output [`ADDR_SIZE-1:0] addr
); 
	
	reg [`ADDR_SIZE-1:0] addr_tmp;  
	reg direction_tmp;

	//Local (distributed) memory
	reg [`WORD_SIZE-1:0] matrixA[`ROW_SIZE-1:0][`COL_SIZE-1:0];

	assign addr = addr_tmp;
	assign direction = direction_tmp;

	always @(posedge clk)  
	begin  
		if (rst)
		begin
			addr_tmp <= 0;
			direction_tmp <= 0;
		end
		else
		begin
			direction_tmp = ~direction_tmp;
			if( direction_tmp )
			begin
				if(addr_tmp < (`RAM_DEPTH-1) )
					addr_tmp = addr_tmp + 1;  
				else
					addr_tmp = 0;  
				data_out = matrixA[ addr_tmp / `COL_SIZE ][ addr_tmp - ((addr_tmp / `COL_SIZE)*`COL_SIZE) ];
			end
			else
			begin
				matrixA[ addr_tmp / `COL_SIZE ][ addr_tmp - ((addr_tmp / `COL_SIZE)*`COL_SIZE) ] = addr_tmp;
			end
		end
	end  
			 
 endmodule
