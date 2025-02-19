`timescale 1ns / 1ps

module two_kmeans_with_noc (
    input wire clk,
    input wire resetn,
    output wire dummy_o
);

    assign dummy_o = mem_axi_rvalid;

    // AXI Master-Slave Instance
    wire [63:0]  mem_axi_awaddr;
    wire [7:0]   mem_axi_awlen;
    wire [2:0]   mem_axi_awsize;
    wire [1:0]   mem_axi_awburst;
    wire         mem_axi_awvalid;
    wire         mem_axi_awready;
    wire [511:0] mem_axi_wdata;
    wire [63:0]  mem_axi_wstrb;
    wire         mem_axi_wlast;
    wire         mem_axi_wvalid;
    wire         mem_axi_wready;
    wire [1:0]   mem_axi_bresp;
    wire         mem_axi_bvalid;
    wire         mem_axi_bready;
    wire [63:0]  mem_axi_araddr;
    wire [7:0]   mem_axi_arlen;
    wire [2:0]   mem_axi_arsize;
    wire [1:0]   mem_axi_arburst;
    wire         mem_axi_arvalid;
    wire         mem_axi_arready;
    wire [511:0] mem_axi_rdata;
    wire [1:0]   mem_axi_rresp;
    wire         mem_axi_rlast;
    wire         mem_axi_rvalid;
    wire         mem_axi_rready;

    wire [63:0]  cfg_axi_awaddr;
    wire [7:0]   cfg_axi_awlen;
    wire [2:0]   cfg_axi_awsize;
    wire [1:0]   cfg_axi_awburst;
    wire         cfg_axi_awvalid;
    wire         cfg_axi_awready;
    wire [511:0] cfg_axi_wdata;
    wire [63:0]  cfg_axi_wstrb;
    wire         cfg_axi_wlast;
    wire         cfg_axi_wvalid;
    wire         cfg_axi_wready;
    wire [1:0]   cfg_axi_bresp;
    wire         cfg_axi_bvalid;
    wire         cfg_axi_bready;
    wire [63:0]  cfg_axi_araddr;
    wire [7:0]   cfg_axi_arlen;
    wire [2:0]   cfg_axi_arsize;
    wire [1:0]   cfg_axi_arburst;
    wire         cfg_axi_arvalid;
    wire         cfg_axi_arready;
    wire [511:0] cfg_axi_rdata;
    wire [1:0]   cfg_axi_rresp;
    wire         cfg_axi_rlast;
    wire         cfg_axi_rvalid;
    wire         cfg_axi_rready;

    noc_router_module noc_router_module_inst (
        .clk(clk),
        .resetn(resetn),

        .s_axi_awaddr(mem_axi_awaddr),
        .s_axi_awlen(mem_axi_awlen),
        .s_axi_awsize(mem_axi_awsize),
        .s_axi_awburst(mem_axi_awburst),
        .s_axi_awvalid(mem_axi_awvalid),
        .s_axi_awready(mem_axi_awready),
        .s_axi_wdata(mem_axi_wdata),
        .s_axi_wstrb(mem_axi_wstrb),
        .s_axi_wlast(mem_axi_wlast),
        .s_axi_wvalid(mem_axi_wvalid),
        .s_axi_wready(mem_axi_wready),
        .s_axi_bresp(mem_axi_bresp),
        .s_axi_bvalid(mem_axi_bvalid),
        .s_axi_bready(mem_axi_bready),
        .s_axi_araddr(mem_axi_araddr),
        .s_axi_arlen(mem_axi_arlen),
        .s_axi_arsize(mem_axi_arsize),
        .s_axi_arburst(mem_axi_arburst),
        .s_axi_arvalid(mem_axi_arvalid),
        .s_axi_arready(mem_axi_arready),
        .s_axi_rdata(mem_axi_rdata),
        .s_axi_rresp(mem_axi_rresp),
        .s_axi_rlast(mem_axi_rlast),
        .s_axi_rvalid(mem_axi_rvalid),
        .s_axi_rready(mem_axi_rready),

        .m_axi_awaddr(cfg_axi_awaddr),
        .m_axi_awlen(cfg_axi_awlen),
        .m_axi_awsize(cfg_axi_awsize),
        .m_axi_awburst(cfg_axi_awburst),
        .m_axi_awvalid(cfg_axi_awvalid),
        .m_axi_awready(cfg_axi_awready),
        .m_axi_wdata(cfg_axi_wdata),
        .m_axi_wstrb(cfg_axi_wstrb),
        .m_axi_wlast(cfg_axi_wlast),
        .m_axi_wvalid(cfg_axi_wvalid),
        .m_axi_wready(cfg_axi_wready),
        .m_axi_bresp(cfg_axi_bresp),
        .m_axi_bvalid(cfg_axi_bvalid),
        .m_axi_bready(cfg_axi_bready),
        .m_axi_araddr(cfg_axi_araddr),
        .m_axi_arlen(cfg_axi_arlen),
        .m_axi_arsize(cfg_axi_arsize),
        .m_axi_arburst(cfg_axi_arburst),
        .m_axi_arvalid(cfg_axi_arvalid),
        .m_axi_arready(cfg_axi_arready),
        .m_axi_rdata(cfg_axi_rdata),
        .m_axi_rresp(cfg_axi_rresp),
        .m_axi_rlast(cfg_axi_rlast),
        .m_axi_rvalid(cfg_axi_rvalid),
        .m_axi_rready(cfg_axi_rready)
    );

    // two_kmeans Instance
    two_kmeans two_kmeans_inst (
        .clk(clk),
        .rst(~resetn),

        // Slave AXI interface
        .s_axi_awid(8'd0),
        .s_axi_awaddr(cfg_axi_awaddr[7:0]),
        .s_axi_awlen(cfg_axi_awlen),
        .s_axi_awsize(cfg_axi_awsize),
        .s_axi_awburst(cfg_axi_awburst),
        .s_axi_awlock(1'b0),
        .s_axi_awcache(4'd0),
        .s_axi_awprot(3'd0),
        .s_axi_awvalid(cfg_axi_awvalid),
        .s_axi_awready(cfg_axi_awready),
        .s_axi_wdata(cfg_axi_wdata[31:0]),
        .s_axi_wstrb(cfg_axi_wstrb[3:0]),
        .s_axi_wlast(cfg_axi_wlast),
        .s_axi_wvalid(cfg_axi_wvalid),
        .s_axi_wready(cfg_axi_wready),
        .s_axi_bid(),
        .s_axi_bresp(cfg_axi_bresp),
        .s_axi_bvalid(cfg_axi_bvalid),
        .s_axi_bready(cfg_axi_bready),
        .s_axi_arid(8'd0),
        .s_axi_araddr(cfg_axi_araddr[7:0]),
        .s_axi_arlen(cfg_axi_arlen),
        .s_axi_arsize(cfg_axi_arsize),
        .s_axi_arburst(cfg_axi_arburst),
        .s_axi_arlock(1'b0),
        .s_axi_arcache(4'd0),
        .s_axi_arprot(3'd0),
        .s_axi_arvalid(cfg_axi_arvalid),
        .s_axi_arready(cfg_axi_arready),
        .s_axi_rid(),
        .s_axi_rdata(cfg_axi_rdata[31:0]),
        .s_axi_rresp(cfg_axi_rresp),
        .s_axi_rlast(cfg_axi_rlast),
        .s_axi_rvalid(cfg_axi_rvalid),
        .s_axi_rready(cfg_axi_rready),

        // Master AXI interface
        .m00_axi_awid(),
        .m00_axi_awaddr(mem_axi_awaddr),
        .m00_axi_awlen(mem_axi_awlen),
        .m00_axi_awsize(mem_axi_awsize),
        .m00_axi_awburst(mem_axi_awburst),
        .m00_axi_awlock(),
        .m00_axi_awcache(),
        .m00_axi_awprot(),
        .m00_axi_awqos(),
        .m00_axi_awregion(),
        .m00_axi_awuser(),
        .m00_axi_awvalid(mem_axi_awvalid),
        .m00_axi_awready(mem_axi_awready),
        .m00_axi_wdata(mem_axi_wdata),
        .m00_axi_wstrb(mem_axi_wstrb[7:0]),
        .m00_axi_wlast(mem_axi_wlast),
        .m00_axi_wuser(),
        .m00_axi_wvalid(mem_axi_wvalid),
        .m00_axi_wready(mem_axi_wready),
        .m00_axi_bid(),
        .m00_axi_bresp(mem_axi_bresp),
        .m00_axi_buser(),
        .m00_axi_bvalid(mem_axi_bvalid),
        .m00_axi_bready(mem_axi_bready),
        .m00_axi_arid(),
        .m00_axi_araddr(mem_axi_araddr),
        .m00_axi_arlen(mem_axi_arlen),
        .m00_axi_arsize(mem_axi_arsize),
        .m00_axi_arburst(mem_axi_arburst),
        .m00_axi_arlock(),
        .m00_axi_arcache(),
        .m00_axi_arprot(),
        .m00_axi_arqos(),
        .m00_axi_arregion(),
        .m00_axi_aruser(),
        .m00_axi_arvalid(mem_axi_arvalid),
        .m00_axi_arready(mem_axi_arready),
        .m00_axi_rid(),
        .m00_axi_rdata(mem_axi_rdata),
        .m00_axi_rresp(mem_axi_rresp),
        .m00_axi_rlast(mem_axi_rlast),
        .m00_axi_ruser(),
        .m00_axi_rvalid(mem_axi_rvalid),
        .m00_axi_rready(mem_axi_rready)
    );

endmodule