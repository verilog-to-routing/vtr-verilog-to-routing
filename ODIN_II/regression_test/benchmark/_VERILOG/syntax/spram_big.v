// DEFINES
`define WIDTH 16         // Bit width
`define DEPTH 8         // Bit depth

module  spram_big(
	clock,
	reset_n,
	value_out,
	value_in
);

/*  Input Declaration */
	input   clock;
	input   reset_n;
	input 	[`WIDTH-1:0]	value_in;

	/*  Output Declaration */
	output	[`WIDTH-1:0] 	value_out;

	reg		[`DEPTH-1:0] address_counter;
	reg		temp_reset;

    defparam inst1.ADDR_WIDTH = `DEPTH;
    defparam inst1.DATA_WIDTH = `WIDTH;
	single_port_ram inst1(
		.clk	( clock ),
		.we		( clock ),
		.data	( value_in ),
		.out	( value_out ),
		.addr	( address_counter )
	);

	always @(posedge clock)
	begin
		if(	temp_reset != 0 )
		begin
			temp_reset <= 0;
			address_counter <= 0;
		end
		else if (reset_n == 1)
			address_counter <= 0;
		else
			address_counter <= address_counter + 1;
		
	end

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
