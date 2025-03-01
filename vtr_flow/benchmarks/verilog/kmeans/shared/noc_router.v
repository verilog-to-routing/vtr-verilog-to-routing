`timescale 1ns / 1ps

module noc_router_adapter (
    // Global Signals
    input wire clk,
    input wire resetn,

    // Slave AXI Interface
    input  wire [63:0]  s_axi_awaddr,
    input  wire [7:0]   s_axi_awlen,
    input  wire [2:0]   s_axi_awsize,
    input  wire [1:0]   s_axi_awburst,
    input  wire         s_axi_awvalid,
    output reg         s_axi_awready, /* synthesis preserve */ 
    input  wire [511:0] s_axi_wdata,
    input  wire [63:0]  s_axi_wstrb,
    input  wire         s_axi_wlast,
    input  wire         s_axi_wvalid,
    output reg         s_axi_wready, /* synthesis preserve */
    output reg [1:0]   s_axi_bresp, /* synthesis preserve */
    output reg        s_axi_bvalid, /* synthesis preserve */
    input  wire        s_axi_bready,
    input  wire [63:0]  s_axi_araddr,
    input  wire [7:0]   s_axi_arlen,
    input  wire [2:0]   s_axi_arsize,
    input  wire [1:0]   s_axi_arburst,
    input  wire         s_axi_arvalid,
    output reg         s_axi_arready, /* synthesis preserve */
    output reg [511:0] s_axi_rdata, /* synthesis preserve */
    output reg [1:0]   s_axi_rresp, /* synthesis preserve */
    output reg         s_axi_rlast, /* synthesis preserve */
    output reg         s_axi_rvalid, /* synthesis preserve */
    input  wire         s_axi_rready
);

wire clk_1; /* synthesis keep */


always @(posedge clk_1) begin
	s_axi_awready <= 0;
	s_axi_wready <= 0;
	s_axi_bresp <= 0;
	s_axi_bvalid <= 0;
	s_axi_arready <= 0;
	s_axi_rdata <= 0;
	s_axi_rresp <= 0;
	s_axi_rlast <= 0;
	s_axi_rvalid <= 0;
end

endmodule