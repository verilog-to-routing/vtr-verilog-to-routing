`define BITS 4
`define ADDR_SIZE 4

module v_runGA (
	clk,
  output_data,
  addr_req,
);

	input clk;
  input [`ADDR_SIZE-1:0] addr_req;

  output [`BITS-1:0] output_data;

	bramctrlsimple
	#(
		.ADDR_WIDTH(`ADDR_SIZE),
		.DATA_WIDTH(`BITS)
	)
	mem1(
		.clk(clk),
    .mem_access(1'b1),
    .mem_data_out(output_data),
    .mem_addr_in(addr_req),
    .mem_data_in(addr_req),
	  .mem_req(1'b1)
	);

endmodule

module bramctrlsimple (
  clk, 
  mem_access, 
  mem_data_out, 
  mem_addr_in, 
  mem_data_in, 
  mem_req
);

  parameter DATA_WIDTH = 5;
  parameter ADDR_WIDTH = 5;

  input	                   clk;
  input                    mem_req;
  input                    mem_access;
  input 	[DATA_WIDTH-1:0] mem_data_in;
  input 	[ADDR_WIDTH-1:0] mem_addr_in;
  output  [DATA_WIDTH-1:0] mem_data_out;

  wire 	we;
  assign we = (mem_access == 1) ? mem_req : 0;

  single_port_ram inst1(
		.clk	( clk ),
		.we		( we ),
		.data	( mem_data_in ),
		.out	( mem_data_out ),
		.addr	( mem_addr_in )
	);

endmodule

/**
 * Copying the modules Single Port RAM from vtr_flow/primitives.v 
 * to correct the hard block inferrence by Yosys 
*/
//single_port_ram module
module single_port_ram #(
    parameter ADDR_WIDTH = 1,
    parameter DATA_WIDTH = 1
) (
    input clk,
    input [ADDR_WIDTH-1:0] addr,
    input [DATA_WIDTH-1:0] data,
    input we,
    output reg [DATA_WIDTH-1:0] out
);

    localparam MEM_DEPTH = 2 ** ADDR_WIDTH;

    reg [DATA_WIDTH-1:0] Mem[MEM_DEPTH-1:0];

    specify
        (clk*>out)="";
        $setup(addr, posedge clk, "");
        $setup(data, posedge clk, "");
        $setup(we, posedge clk, "");
        $hold(posedge clk, addr, "");
        $hold(posedge clk, data, "");
        $hold(posedge clk, we, "");
    endspecify
   
    always@(posedge clk) begin
        if(we) begin
            Mem[addr] = data;
        end
    	out = Mem[addr]; //New data read-during write behaviour (blocking assignments)
    end
   
endmodule // single_port_RAM