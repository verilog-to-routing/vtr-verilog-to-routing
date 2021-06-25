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

	defparam sp_ram.ADDR_WIDTH = `MEMORY_CONTROLLER_ADDR_SIZE;
	defparam sp_ram.DATA_WIDTH = `MEMORY_CONTROLLER_DATA_SIZE;
	single_port_ram sp_ram (		
		.addr (addr1),
		.data (data1),
		.we   (we1),	
		.clk  (clk),	
		.out  (spi_out)
	);

	defparam dp_ram.ADDR_WIDTH = `MEMORY_CONTROLLER_ADDR_SIZE;
	defparam dp_ram.DATA_WIDTH = `MEMORY_CONTROLLER_DATA_SIZE;
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

/**
 * Copying the modules Single Port RAM and Dual Port RAM from vtr_flow/primitives.v 
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