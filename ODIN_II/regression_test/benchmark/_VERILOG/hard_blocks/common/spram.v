module spram_instance(
	address,
	value_in,
	we,
	clock,
	value_out
);

	parameter WIDTH = 16;	// Bit width
	parameter DEPTH = 8;	// Bit depth

/*  Input Declaration */
	input	[DEPTH-1:0] 	address;
	input 	[WIDTH-1:0]		value_in;
	input 	we;
	input   clock;

	/*  Output Declaration */
	output	[WIDTH-1:0] 	value_out;

	defparam inst1.ADDR_WIDTH = DEPTH; 
	defparam inst1.DATA_WIDTH = WIDTH;
	single_port_ram inst1 (
		.addr	( address ),
		.data	( value_in ),
		.we		( clock ),
		.clk    ( clock ),
		.out	( value_out )
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