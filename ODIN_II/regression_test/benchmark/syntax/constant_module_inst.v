`define BITS 3
`define MEMORY_WORDS 11
`define MEM_ACCES_BIT_SIZE 4

module bramctrlsimple (clk, mem_done, mem_access, mem_data_out, mem_addr_in, mem_data_in, mem_req);

  parameter SIZE = 1000;
  parameter DATA_WIDTH = 32;
  parameter ADDR_WIDTH = 32;

  input	                   clk;
  input                    mem_req;
  input                    mem_access;
  input 	[DATA_WIDTH-1:0] mem_data_in;
  input 	[ADDR_WIDTH-1:0] mem_addr_in;
  output  [DATA_WIDTH-1:0] mem_data_out;
  output                   mem_done;

  wire 	we;
  reg [DATA_WIDTH-1:0] bram [SIZE-1:0];

  assign we = (mem_access == 1) ? mem_req : 0;

  always @(posedge clk) begin
     mem_done <= mem_req;
  end

  always @(posedge clk)
  begin
  	if (we == 1)
  		bram[mem_addr_in] <= mem_data_in;

    mem_data_out <= bram[mem_addr_in];
  end
endmodule

module v_runGA (
	clk,
  output_data,
  input_data,
  addr_req,
  req,
	);

	input clk;
  input req;
  input [`MEM_ACCES_BIT_SIZE-1:0] addr_req;
  input [`BITS-1:0] input_data;
  output [`BITS-1:0] output_data;
  reg [`MEM_ACCES_BIT_SIZE-1:0] temp;

  assign temp = 0;

	bramctrlsimple
	#(
		.SIZE(`MEMORY_WORDS),
		.ADDR_WIDTH(`MEM_ACCES_BIT_SIZE),
		.DATA_WIDTH(`BITS)
	)
	mem1(
		.clk(clk),
		.mem_done(1),
    .mem_access(0),
    .mem_data_out(output_data),
    .mem_addr_in(addr_req),
    .mem_data_in(1'b1),
	  .mem_req(req)
	);

endmodule
