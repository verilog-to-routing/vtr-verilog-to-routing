module dpram_instance(
	address1,
	address2,
	value_in1,
	value_in2,
	we1,
	we2,
	clock,
	value_out1,
	value_out2
);

	parameter WIDTH = 16;	// Bit width
	parameter DEPTH = 8;	// Bit depth

/*  Input Declaration */
	input	[DEPTH-1:0] 	address1;
	input	[DEPTH-1:0] 	address2;
	input 	[WIDTH-1:0]		value_in1;
	input 	[WIDTH-1:0]		value_in2;
	input 	we1;
	input 	we2;
	input   clock;

	/*  Output Declaration */
	output	[WIDTH-1:0] 	value_out1;
	output	[WIDTH-1:0] 	value_out2;

	defparam inst1.ADDR_WIDTH = DEPTH; 
	defparam inst1.DATA_WIDTH = WIDTH;
	dual_port_ram inst1 (
		.addr1	( address1 ),
		.addr2	( address2 ),
		.data1	( value_in1 ),
		.data2	( value_in2 ),
		.we1	( clock ),
		.we2	( clock ),
		.clk    ( clock ),
		.out1	( value_out1 ),
		.out2	( value_out2 )
	);

endmodule

/**
 * Copying the modules Dual Port RAM from vtr_flow/primitives.v 
 * to correct the hard block inferrence by Yosys 
*/
//dual_port_ram module
module dual_port_ram #(
    parameter ADDR_WIDTH = 1,
    parameter DATA_WIDTH = 1
) (
    input clk,

    input [ADDR_WIDTH-1:0] addr1,
    input [ADDR_WIDTH-1:0] addr2,
    input [DATA_WIDTH-1:0] data1,
    input [DATA_WIDTH-1:0] data2,
    input we1,
    input we2,
    output reg [DATA_WIDTH-1:0] out1,
    output reg [DATA_WIDTH-1:0] out2
);

    localparam MEM_DEPTH = 2 ** ADDR_WIDTH;

    reg [DATA_WIDTH-1:0] Mem[MEM_DEPTH-1:0];

    specify
        (clk*>out1)="";
        (clk*>out2)="";
        $setup(addr1, posedge clk, "");
        $setup(addr2, posedge clk, "");
        $setup(data1, posedge clk, "");
        $setup(data2, posedge clk, "");
        $setup(we1, posedge clk, "");
        $setup(we2, posedge clk, "");
        $hold(posedge clk, addr1, "");
        $hold(posedge clk, addr2, "");
        $hold(posedge clk, data1, "");
        $hold(posedge clk, data2, "");
        $hold(posedge clk, we1, "");
        $hold(posedge clk, we2, "");
    endspecify
   
    always@(posedge clk) begin //Port 1
        if(we1) begin
            Mem[addr1] = data1;
        end
        out1 = Mem[addr1]; //New data read-during write behaviour (blocking assignments)
    end

    always@(posedge clk) begin //Port 2
        if(we2) begin
            Mem[addr2] = data2;
        end
        out2 = Mem[addr2]; //New data read-during write behaviour (blocking assignments)
    end
   
endmodule // dual_port_ram