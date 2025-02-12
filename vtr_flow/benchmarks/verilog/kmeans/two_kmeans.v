
module top (
    input  wire                        clk,
    input  wire                        rst,

    /*
     * AXI slave interface
     */
    input  wire [7:0]                  s_axi_awid,
    input  wire [7:0]                  s_axi_awaddr,
    input  wire [7:0]                  s_axi_awlen,
    input  wire [2:0]                  s_axi_awsize,
    input  wire [1:0]                  s_axi_awburst,
    input  wire                        s_axi_awlock,
    input  wire [3:0]                  s_axi_awcache,
    input  wire [2:0]                  s_axi_awprot,
    input  wire                        s_axi_awvalid,
    output wire                        s_axi_awready,
    input  wire [31:0]   s_axi_wdata,
    input  wire [3:0]   s_axi_wstrb,
    input  wire                        s_axi_wlast,
    input  wire                        s_axi_wvalid,
    output wire                        s_axi_wready,
    output wire [7:0]     s_axi_bid,
    output wire [1:0]                  s_axi_bresp,
    output wire                        s_axi_bvalid,
    input  wire                        s_axi_bready,
    input  wire [7:0]     s_axi_arid,
    input  wire [7:0]       s_axi_araddr,
    input  wire [7:0]                  s_axi_arlen,
    input  wire [2:0]                  s_axi_arsize,
    input  wire [1:0]                  s_axi_arburst,
    input  wire                        s_axi_arlock,
    input  wire [3:0]                  s_axi_arcache,
    input  wire [2:0]                  s_axi_arprot,
    input  wire                        s_axi_arvalid,
    output wire                        s_axi_arready,
    output wire [7:0]     s_axi_rid,
    output wire [31:0]   s_axi_rdata,
    output wire [1:0]                  s_axi_rresp,
    output wire                        s_axi_rlast,
    output wire                        s_axi_rvalid,
    input  wire                        s_axi_rready,


    /*
     * AXI master interface
     */
    output wire [7:0]      m00_axi_awid,
    output wire [63:0]    m00_axi_awaddr,
    output wire [7:0]               m00_axi_awlen,
    output wire [2:0]               m00_axi_awsize,
    output wire [1:0]               m00_axi_awburst,
    output wire                     m00_axi_awlock,
    output wire [3:0]               m00_axi_awcache,
    output wire [2:0]               m00_axi_awprot,
    output wire [3:0]               m00_axi_awqos,
    output wire [3:0]               m00_axi_awregion,
    output wire [0:0]  m00_axi_awuser,
    output wire                     m00_axi_awvalid,
    input  wire                     m00_axi_awready,
    output wire [63:0]    m00_axi_wdata,
    output wire [7:0]    m00_axi_wstrb,
    output wire                     m00_axi_wlast,
    output wire [0:0]   m00_axi_wuser,
    output wire                     m00_axi_wvalid,
    input  wire                     m00_axi_wready,
    input  wire [7:0]      m00_axi_bid,
    input  wire [1:0]               m00_axi_bresp,
    input  wire [0:0]   m00_axi_buser,
    input  wire                     m00_axi_bvalid,
    output wire                     m00_axi_bready,
    output wire [7:0]      m00_axi_arid,
    output wire [63:0]    m00_axi_araddr,
    output wire [7:0]               m00_axi_arlen,
    output wire [2:0]               m00_axi_arsize,
    output wire [1:0]               m00_axi_arburst,
    output wire                     m00_axi_arlock,
    output wire [3:0]               m00_axi_arcache,
    output wire [2:0]               m00_axi_arprot,
    output wire [3:0]               m00_axi_arqos,
    output wire [3:0]               m00_axi_arregion,
    output wire [0:0]  m00_axi_aruser,
    output wire                     m00_axi_arvalid,
    input  wire                     m00_axi_arready,
    input  wire [7:0]      m00_axi_rid,
    input  wire [63:0]    m00_axi_rdata,
    input  wire [1:0]               m00_axi_rresp,
    input  wire                     m00_axi_rlast,
    input  wire [0:0]   m00_axi_ruser,
    input  wire                     m00_axi_rvalid,
    output wire                     m00_axi_rready

);

    wire ap_clk;
    wire ap_rst_n;

    assign ap_clk = clk;
    assign ap_rst_n = ~rst;
    
    wire axi0_mem_AWVALID;
    wire axi0_mem_AWREADY;
    wire [63:0] axi0_mem_AWADDR;
    wire [0:0] axi0_mem_AWID;
    wire [7:0] axi0_mem_AWLEN;
    wire [2:0] axi0_mem_AWSIZE;
    wire [1:0] axi0_mem_AWBURST;
    wire [1:0] axi0_mem_AWLOCK;
    wire [3:0] axi0_mem_AWCACHE;
    wire [2:0] axi0_mem_AWPROT;
    wire [3:0] axi0_mem_AWQOS;
    wire [3:0] axi0_mem_AWREGION;
    wire [0:0] axi0_mem_AWUSER;
    
    wire axi0_mem_WVALID;
    wire axi0_mem_WREADY;
    wire [63:0] axi0_mem_WDATA;
    wire [7:0] axi0_mem_WSTRB;
    wire axi0_mem_WLAST;
    wire [0:0] axi0_mem_WID;
    wire [0:0] axi0_mem_WUSER;
    
    wire axi0_mem_ARVALID;
    wire axi0_mem_ARREADY;
    wire [63:0] axi0_mem_ARADDR;
    wire [0:0] axi0_mem_ARID;
    wire [7:0] axi0_mem_ARLEN;
    wire [2:0] axi0_mem_ARSIZE;
    wire [1:0] axi0_mem_ARBURST;
    wire [1:0] axi0_mem_ARLOCK;
    wire [3:0] axi0_mem_ARCACHE;
    wire [2:0] axi0_mem_ARPROT;
    wire [3:0] axi0_mem_ARQOS;
    wire [3:0] axi0_mem_ARREGION;
    wire [0:0] axi0_mem_ARUSER;
    
    wire axi0_mem_RVALID;
    wire axi0_mem_RREADY;
    wire [63:0] axi0_mem_RDATA;
    wire axi0_mem_RLAST;
    wire [0:0] axi0_mem_RID;
    wire [0:0] axi0_mem_RUSER;
    wire [1:0] axi0_mem_RRESP;
    
    wire axi0_mem_BVALID;
    wire axi0_mem_BREADY;
    wire [1:0] axi0_mem_BRESP;
    wire [0:0] axi0_mem_BID;
    wire [0:0] axi0_mem_BUSER;
    
    wire axi0_cfg_AWVALID;
    wire axi0_cfg_AWREADY;
    wire [7:0] axi0_cfg_AWADDR;
    wire axi0_cfg_WVALID;
    wire axi0_cfg_WREADY;
    wire [31:0] axi0_cfg_WDATA;
    wire [3:0] axi0_cfg_WSTRB;
    wire axi0_cfg_ARVALID;
    wire axi0_cfg_ARREADY;
    wire [7:0] axi0_cfg_ARADDR;
    
    wire axi0_cfg_RVALID;
    wire axi0_cfg_RREADY;
    wire [31:0] axi0_cfg_RDATA;
    wire [1:0] axi0_cfg_RRESP;
    
    wire axi0_cfg_BVALID;
    wire axi0_cfg_BREADY;
    wire [1:0] axi0_cfg_BRESP;
    wire interrupt0;

    wire axi1_mem_AWVALID;
    wire axi1_mem_AWREADY;
    wire [63:0] axi1_mem_AWADDR;
    wire [0:0] axi1_mem_AWID;
    wire [7:0] axi1_mem_AWLEN;
    wire [2:0] axi1_mem_AWSIZE;
    wire [1:0] axi1_mem_AWBURST;
    wire [1:0] axi1_mem_AWLOCK;
    wire [3:0] axi1_mem_AWCACHE;
    wire [2:0] axi1_mem_AWPROT;
    wire [3:0] axi1_mem_AWQOS;
    wire [3:0] axi1_mem_AWREGION;
    wire [0:0] axi1_mem_AWUSER;

    wire axi1_mem_WVALID;
    wire axi1_mem_WREADY;
    wire [63:0] axi1_mem_WDATA;
    wire [7:0] axi1_mem_WSTRB;
    wire axi1_mem_WLAST;
    wire [0:0] axi1_mem_WID;
    wire [0:0] axi1_mem_WUSER;

    wire axi1_mem_ARVALID;
    wire axi1_mem_ARREADY;
    wire [63:0] axi1_mem_ARADDR;
    wire [0:0] axi1_mem_ARID;
    wire [7:0] axi1_mem_ARLEN;
    wire [2:0] axi1_mem_ARSIZE;
    wire [1:0] axi1_mem_ARBURST;
    wire [1:0] axi1_mem_ARLOCK;
    wire [3:0] axi1_mem_ARCACHE;
    wire [2:0] axi1_mem_ARPROT;
    wire [3:0] axi1_mem_ARQOS;
    wire [3:0] axi1_mem_ARREGION;
    wire [0:0] axi1_mem_ARUSER;

    wire axi1_mem_RVALID;
    wire axi1_mem_RREADY;
    wire [63:0] axi1_mem_RDATA;
    wire axi1_mem_RLAST;
    wire [0:0] axi1_mem_RID;
    wire [0:0] axi1_mem_RUSER;
    wire [1:0] axi1_mem_RRESP;

    wire axi1_mem_BVALID;
    wire axi1_mem_BREADY;
    wire [1:0] axi1_mem_BRESP;
    wire [0:0] axi1_mem_BID;
    wire [0:0] axi1_mem_BUSER;

    wire axi1_cfg_AWVALID;
    wire axi1_cfg_AWREADY;
    wire [7:0] axi1_cfg_AWADDR;
    wire axi1_cfg_WVALID;
    wire axi1_cfg_WREADY;
    wire [31:0] axi1_cfg_WDATA;
    wire [3:0] axi1_cfg_WSTRB;
    wire axi1_cfg_ARVALID;
    wire axi1_cfg_ARREADY;
    wire [7:0] axi1_cfg_ARADDR;

    wire axi1_cfg_RVALID;
    wire axi1_cfg_RREADY;
    wire [31:0] axi1_cfg_RDATA;
    wire [1:0] axi1_cfg_RRESP;

    wire axi1_cfg_BVALID;
    wire axi1_cfg_BREADY;
    wire [1:0] axi1_cfg_BRESP;
    wire interrupt1;

    wire axi_shared_cfg_AWVALID;
    wire axi_shared_cfg_AWREADY;
    wire [7:0] axi_shared_cfg_AWADDR;
    wire axi_shared_cfg_WVALID;
    wire axi_shared_cfg_WREADY;
    wire [31:0] axi_shared_cfg_WDATA;
    wire [3:0] axi_shared_cfg_WSTRB;
    wire axi_shared_cfg_ARVALID;
    wire axi_shared_cfg_ARREADY;
    wire [7:0] axi_shared_cfg_ARADDR;
    wire axi_shared_cfg_RVALID;
    wire axi_shared_cfg_RREADY;
    wire [31:0] axi_shared_cfg_RDATA;
    wire [1:0] axi_shared_cfg_RRESP;
    wire axi_shared_cfg_BVALID;
    wire axi_shared_cfg_BREADY;
    wire [1:0] axi_shared_cfg_BRESP;
    
    kmeans_flat kmeans_inst0 (
        .ap_clk(ap_clk),
        .ap_rst_n(ap_rst_n),

        .m_axi_mem_AWVALID(axi0_mem_AWVALID),
        .m_axi_mem_AWREADY(axi0_mem_AWREADY),
        .m_axi_mem_AWADDR(axi0_mem_AWADDR),
        .m_axi_mem_AWID(axi0_mem_AWID),
        .m_axi_mem_AWLEN(axi0_mem_AWLEN),
        .m_axi_mem_AWSIZE(axi0_mem_AWSIZE),
        .m_axi_mem_AWBURST(axi0_mem_AWBURST),
        .m_axi_mem_AWLOCK(axi0_mem_AWLOCK),
        .m_axi_mem_AWCACHE(axi0_mem_AWCACHE),
        .m_axi_mem_AWPROT(axi0_mem_AWPROT),
        .m_axi_mem_AWQOS(axi0_mem_AWQOS),
        .m_axi_mem_AWREGION(axi0_mem_AWREGION),
        .m_axi_mem_AWUSER(axi0_mem_AWUSER),
        .m_axi_mem_WVALID(axi0_mem_WVALID),
        .m_axi_mem_WREADY(axi0_mem_WREADY),
        .m_axi_mem_WDATA(axi0_mem_WDATA),
        .m_axi_mem_WSTRB(axi0_mem_WSTRB),
        .m_axi_mem_WLAST(axi0_mem_WLAST),
        .m_axi_mem_WID(axi0_mem_WID),
        .m_axi_mem_WUSER(axi0_mem_WUSER),
        .m_axi_mem_ARVALID(axi0_mem_ARVALID),
        .m_axi_mem_ARREADY(axi0_mem_ARREADY),
        .m_axi_mem_ARADDR(axi0_mem_ARADDR),
        .m_axi_mem_ARID(axi0_mem_ARID),
        .m_axi_mem_ARLEN(axi0_mem_ARLEN),
        .m_axi_mem_ARSIZE(axi0_mem_ARSIZE),
        .m_axi_mem_ARBURST(axi0_mem_ARBURST),
        .m_axi_mem_ARLOCK(axi0_mem_ARLOCK),
        .m_axi_mem_ARCACHE(axi0_mem_ARCACHE),
        .m_axi_mem_ARPROT(axi0_mem_ARPROT),
        .m_axi_mem_ARQOS(axi0_mem_ARQOS),
        .m_axi_mem_ARREGION(axi0_mem_ARREGION),
        .m_axi_mem_ARUSER(axi0_mem_ARUSER),
        .m_axi_mem_RVALID(axi0_mem_RVALID),
        .m_axi_mem_RREADY(axi0_mem_RREADY),
        .m_axi_mem_RDATA(axi0_mem_RDATA),
        .m_axi_mem_RLAST(axi0_mem_RLAST),
        .m_axi_mem_RID(axi0_mem_RID),
        .m_axi_mem_RUSER(axi0_mem_RUSER),
        .m_axi_mem_RRESP(axi0_mem_RRESP),
        .m_axi_mem_BVALID(axi0_mem_BVALID),
        .m_axi_mem_BREADY(axi0_mem_BREADY),
        .m_axi_mem_BRESP(axi0_mem_BRESP),
        .m_axi_mem_BID(axi0_mem_BID),
        .m_axi_mem_BUSER(axi0_mem_BUSER),

        .s_axi_cfg_AWVALID(axi0_cfg_AWVALID),
        .s_axi_cfg_AWREADY(axi0_cfg_AWREADY),
        .s_axi_cfg_AWADDR(axi0_cfg_AWADDR),
        .s_axi_cfg_WVALID(axi0_cfg_WVALID),
        .s_axi_cfg_WREADY(axi0_cfg_WREADY),
        .s_axi_cfg_WDATA(axi0_cfg_WDATA),
        .s_axi_cfg_WSTRB(axi0_cfg_WSTRB),
        .s_axi_cfg_ARVALID(axi0_cfg_ARVALID),
        .s_axi_cfg_ARREADY(axi0_cfg_ARREADY),
        .s_axi_cfg_ARADDR(axi0_cfg_ARADDR),
        .s_axi_cfg_RVALID(axi0_cfg_RVALID),
        .s_axi_cfg_RREADY(axi0_cfg_RREADY),
        .s_axi_cfg_RDATA(axi0_cfg_RDATA),
        .s_axi_cfg_RRESP(axi0_cfg_RRESP),
        .s_axi_cfg_BVALID(axi0_cfg_BVALID),
        .s_axi_cfg_BREADY(axi0_cfg_BREADY),
        .s_axi_cfg_BRESP(axi0_cfg_BRESP),
        .interrupt(interrupt0)
    );

    kmeans_flat kmeans_inst1 (
        .ap_clk(ap_clk),
        .ap_rst_n(ap_rst_n),
        .m_axi_mem_AWVALID(axi1_mem_AWVALID),
        .m_axi_mem_AWREADY(axi1_mem_AWREADY),
        .m_axi_mem_AWADDR(axi1_mem_AWADDR),
        .m_axi_mem_AWID(axi1_mem_AWID),
        .m_axi_mem_AWLEN(axi1_mem_AWLEN),
        .m_axi_mem_AWSIZE(axi1_mem_AWSIZE),
        .m_axi_mem_AWBURST(axi1_mem_AWBURST),
        .m_axi_mem_AWLOCK(axi1_mem_AWLOCK),
        .m_axi_mem_AWCACHE(axi1_mem_AWCACHE),
        .m_axi_mem_AWPROT(axi1_mem_AWPROT),
        .m_axi_mem_AWQOS(axi1_mem_AWQOS),
        .m_axi_mem_AWREGION(axi1_mem_AWREGION),
        .m_axi_mem_AWUSER(axi1_mem_AWUSER),
        .m_axi_mem_WVALID(axi1_mem_WVALID),
        .m_axi_mem_WREADY(axi1_mem_WREADY),
        .m_axi_mem_WDATA(axi1_mem_WDATA),
        .m_axi_mem_WSTRB(axi1_mem_WSTRB),
        .m_axi_mem_WLAST(axi1_mem_WLAST),
        .m_axi_mem_WID(axi1_mem_WID),
        .m_axi_mem_WUSER(axi1_mem_WUSER),
        .m_axi_mem_ARVALID(axi1_mem_ARVALID),
        .m_axi_mem_ARREADY(axi1_mem_ARREADY),
        .m_axi_mem_ARADDR(axi1_mem_ARADDR),
        .m_axi_mem_ARID(axi1_mem_ARID),
        .m_axi_mem_ARLEN(axi1_mem_ARLEN),
        .m_axi_mem_ARSIZE(axi1_mem_ARSIZE),
        .m_axi_mem_ARBURST(axi1_mem_ARBURST),
        .m_axi_mem_ARLOCK(axi1_mem_ARLOCK),
        .m_axi_mem_ARCACHE(axi1_mem_ARCACHE),
        .m_axi_mem_ARPROT(axi1_mem_ARPROT),
        .m_axi_mem_ARQOS(axi1_mem_ARQOS),
        .m_axi_mem_ARREGION(axi1_mem_ARREGION),
        .m_axi_mem_ARUSER(axi1_mem_ARUSER),
        .m_axi_mem_RVALID(axi1_mem_RVALID),
        .m_axi_mem_RREADY(axi1_mem_RREADY),
        .m_axi_mem_RDATA(axi1_mem_RDATA),
        .m_axi_mem_RLAST(axi1_mem_RLAST),
        .m_axi_mem_RID(axi1_mem_RID),
        .m_axi_mem_RUSER(axi1_mem_RUSER),
        .m_axi_mem_RRESP(axi1_mem_RRESP),
        .m_axi_mem_BVALID(axi1_mem_BVALID),
        .m_axi_mem_BREADY(axi1_mem_BREADY),
        .m_axi_mem_BRESP(axi1_mem_BRESP),
        .m_axi_mem_BID(axi1_mem_BID),
        .m_axi_mem_BUSER(axi1_mem_BUSER),
        .s_axi_cfg_AWVALID(axi1_cfg_AWVALID),
        .s_axi_cfg_AWREADY(axi1_cfg_AWREADY),
        .s_axi_cfg_AWADDR(axi1_cfg_AWADDR),
        .s_axi_cfg_WVALID(axi1_cfg_WVALID),
        .s_axi_cfg_WREADY(axi1_cfg_WREADY),
        .s_axi_cfg_WDATA(axi1_cfg_WDATA),
        .s_axi_cfg_WSTRB(axi1_cfg_WSTRB),
        .s_axi_cfg_ARVALID(axi1_cfg_ARVALID),
        .s_axi_cfg_ARREADY(axi1_cfg_ARREADY),
        .s_axi_cfg_ARADDR(axi1_cfg_ARADDR),
        .s_axi_cfg_RVALID(axi1_cfg_RVALID),
        .s_axi_cfg_RREADY(axi1_cfg_RREADY),
        .s_axi_cfg_RDATA(axi1_cfg_RDATA),
        .s_axi_cfg_RRESP(axi1_cfg_RRESP),
        .s_axi_cfg_BVALID(axi1_cfg_BVALID),
        .s_axi_cfg_BREADY(axi1_cfg_BREADY),
        .s_axi_cfg_BRESP(axi1_cfg_BRESP),
        .interrupt(interrupt1)
    );

    axi_interconnect_wrap_2x1 #(
        .DATA_WIDTH(64),
        .ADDR_WIDTH(64),
        .STRB_WIDTH(8), // DATA_WIDTH / 8
        .ID_WIDTH(8),
        .AWUSER_ENABLE(1),
        .AWUSER_WIDTH(1),
        .WUSER_ENABLE(1),
        .WUSER_WIDTH(1),
        .BUSER_ENABLE(1),
        .BUSER_WIDTH(1),
        .ARUSER_ENABLE(1),
        .ARUSER_WIDTH(1),
        .RUSER_ENABLE(1),
        .RUSER_WIDTH(1),
        .FORWARD_ID(1),
        .M_REGIONS(1),
        .M00_BASE_ADDR(0),
        .M00_ADDR_WIDTH({1{32'd64}}), // Single region, 24-bit width
        .M00_CONNECT_READ(2'b11),
        .M00_CONNECT_WRITE(2'b11),
        .M00_SECURE(1'b0)
    ) axi_interconnect_inst (
        .clk(ap_clk),
        .rst(~ap_rst_n),

        // Slave 0 Interface
        .s00_axi_awid(axi0_mem_AWID),
        .s00_axi_awaddr(axi0_mem_AWADDR),
        .s00_axi_awlen(axi0_mem_AWLEN),
        .s00_axi_awsize(axi0_mem_AWSIZE),
        .s00_axi_awburst(axi0_mem_AWBURST),
        .s00_axi_awlock(axi0_mem_AWLOCK),
        .s00_axi_awcache(axi0_mem_AWCACHE),
        .s00_axi_awprot(axi0_mem_AWPROT),
        .s00_axi_awqos(axi0_mem_AWQOS),
        .s00_axi_awuser(axi0_mem_AWUSER),
        .s00_axi_awvalid(axi0_mem_AWVALID),
        .s00_axi_awready(axi0_mem_AWREADY),

        .s00_axi_wdata(axi0_mem_WDATA),
        .s00_axi_wstrb(axi0_mem_WSTRB),
        .s00_axi_wlast(axi0_mem_WLAST),
        .s00_axi_wuser(axi0_mem_WUSER),
        .s00_axi_wvalid(axi0_mem_WVALID),
        .s00_axi_wready(axi0_mem_WREADY),

        .s00_axi_bid(axi0_mem_BID),
        .s00_axi_bresp(axi0_mem_BRESP),
        .s00_axi_buser(axi0_mem_BUSER),
        .s00_axi_bvalid(axi0_mem_BVALID),
        .s00_axi_bready(axi0_mem_BREADY),

        .s00_axi_arid(axi0_mem_ARID),
        .s00_axi_araddr(axi0_mem_ARADDR),
        .s00_axi_arlen(axi0_mem_ARLEN),
        .s00_axi_arsize(axi0_mem_ARSIZE),
        .s00_axi_arburst(axi0_mem_ARBURST),
        .s00_axi_arlock(axi0_mem_ARLOCK),
        .s00_axi_arcache(axi0_mem_ARCACHE),
        .s00_axi_arprot(axi0_mem_ARPROT),
        .s00_axi_arqos(axi0_mem_ARQOS),
        .s00_axi_aruser(axi0_mem_ARUSER),
        .s00_axi_arvalid(axi0_mem_ARVALID),
        .s00_axi_arready(axi0_mem_ARREADY),

        .s00_axi_rid(axi0_mem_RID),
        .s00_axi_rdata(axi0_mem_RDATA),
        .s00_axi_rresp(axi0_mem_RRESP),
        .s00_axi_rlast(axi0_mem_RLAST),
        .s00_axi_ruser(axi0_mem_RUSER),
        .s00_axi_rvalid(axi0_mem_RVALID),
        .s00_axi_rready(axi0_mem_RREADY),

        // Slave 1 Interface
        .s01_axi_awid(axi1_mem_AWID),
        .s01_axi_awaddr(axi1_mem_AWADDR),
        .s01_axi_awlen(axi1_mem_AWLEN),
        .s01_axi_awsize(axi1_mem_AWSIZE),
        .s01_axi_awburst(axi1_mem_AWBURST),
        .s01_axi_awlock(axi1_mem_AWLOCK),
        .s01_axi_awcache(axi1_mem_AWCACHE),
        .s01_axi_awprot(axi1_mem_AWPROT),
        .s01_axi_awqos(axi1_mem_AWQOS),
        .s01_axi_awuser(axi1_mem_AWUSER),
        .s01_axi_awvalid(axi1_mem_AWVALID),
        .s01_axi_awready(axi1_mem_AWREADY),

        .s01_axi_wdata(axi1_mem_WDATA),
        .s01_axi_wstrb(axi1_mem_WSTRB),
        .s01_axi_wlast(axi1_mem_WLAST),
        .s01_axi_wuser(axi1_mem_WUSER),
        .s01_axi_wvalid(axi1_mem_WVALID),
        .s01_axi_wready(axi1_mem_WREADY),

        .s01_axi_bid(axi1_mem_BID),
        .s01_axi_bresp(axi1_mem_BRESP),
        .s01_axi_buser(axi1_mem_BUSER),
        .s01_axi_bvalid(axi1_mem_BVALID),
        .s01_axi_bready(axi1_mem_BREADY),

        .s01_axi_arid(axi1_mem_ARID),
        .s01_axi_araddr(axi1_mem_ARADDR),
        .s01_axi_arlen(axi1_mem_ARLEN),
        .s01_axi_arsize(axi1_mem_ARSIZE),
        .s01_axi_arburst(axi1_mem_ARBURST),
        .s01_axi_arlock(axi1_mem_ARLOCK),
        .s01_axi_arcache(axi1_mem_ARCACHE),
        .s01_axi_arprot(axi1_mem_ARPROT),
        .s01_axi_arqos(axi1_mem_ARQOS),
        .s01_axi_aruser(axi1_mem_ARUSER),
        .s01_axi_arvalid(axi1_mem_ARVALID),
        .s01_axi_arready(axi1_mem_ARREADY),

        .s01_axi_rid(axi1_mem_RID),
        .s01_axi_rdata(axi1_mem_RDATA),
        .s01_axi_rresp(axi1_mem_RRESP),
        .s01_axi_rlast(axi1_mem_RLAST),
        .s01_axi_ruser(axi1_mem_RUSER),
        .s01_axi_rvalid(axi1_mem_RVALID),
        .s01_axi_rready(axi1_mem_RREADY),

        // Master 0 Interface
        .m00_axi_awid(m00_axi_awid),
        .m00_axi_awaddr(m00_axi_awaddr),
        .m00_axi_awlen(m00_axi_awlen),
        .m00_axi_awsize(m00_axi_awsize),
        .m00_axi_awburst(m00_axi_awburst),
        .m00_axi_awlock(m00_axi_awlock),
        .m00_axi_awcache(m00_axi_awcache),
        .m00_axi_awprot(m00_axi_awprot),
        .m00_axi_awqos(m00_axi_awqos),
        .m00_axi_awregion(m00_axi_awregion),
        .m00_axi_awuser(m00_axi_awuser),
        .m00_axi_awvalid(m00_axi_awvalid),
        .m00_axi_awready(m00_axi_awready),
        .m00_axi_wdata(m00_axi_wdata),
        .m00_axi_wstrb(m00_axi_wstrb),
        .m00_axi_wlast(m00_axi_wlast),
        .m00_axi_wuser(m00_axi_wuser),
        .m00_axi_wvalid(m00_axi_wvalid),
        .m00_axi_wready(m00_axi_wready),
        .m00_axi_bid(m00_axi_bid),
        .m00_axi_bresp(m00_axi_bresp),
        .m00_axi_buser(m00_axi_buser),
        .m00_axi_bvalid(m00_axi_bvalid),
        .m00_axi_bready(m00_axi_bready),
        .m00_axi_arid(m00_axi_arid),
        .m00_axi_araddr(m00_axi_araddr),
        .m00_axi_arlen(m00_axi_arlen),
        .m00_axi_arsize(m00_axi_arsize),
        .m00_axi_arburst(m00_axi_arburst),
        .m00_axi_arlock(m00_axi_arlock),
        .m00_axi_arcache(m00_axi_arcache),
        .m00_axi_arprot(m00_axi_arprot),
        .m00_axi_arqos(m00_axi_arqos),
        .m00_axi_arregion(m00_axi_arregion),
        .m00_axi_aruser(m00_axi_aruser),
        .m00_axi_arvalid(m00_axi_arvalid),
        .m00_axi_arready(m00_axi_arready),
        .m00_axi_rid(m00_axi_rid),
        .m00_axi_rdata(m00_axi_rdata),
        .m00_axi_rresp(m00_axi_rresp),
        .m00_axi_rlast(m00_axi_rlast),
        .m00_axi_ruser(m00_axi_ruser),
        .m00_axi_rvalid(m00_axi_rvalid),
        .m00_axi_rready(m00_axi_rready)
    );


    axil_interconnect_wrap_1x2 #(
        .DATA_WIDTH(32),
        .ADDR_WIDTH(8),
        .STRB_WIDTH(4), // DATA_WIDTH / 8
        .M_REGIONS(1),
        .M00_BASE_ADDR(8'h00), // Base address for Master 0
        .M00_ADDR_WIDTH({32'd24}), // Address width for Master 0
        .M00_CONNECT_READ(1'b1),  // Enable read connections for Master 0
        .M00_CONNECT_WRITE(1'b1), // Enable write connections for Master 0
        .M00_SECURE(1'b0),        // Security setting for Master 0
        .M01_BASE_ADDR(8'h80),    // Base address for Master 1
        .M01_ADDR_WIDTH({32'd24}), // Address width for Master 1
        .M01_CONNECT_READ(1'b1),  // Enable read connections for Master 1
        .M01_CONNECT_WRITE(1'b1), // Enable write connections for Master 1
        .M01_SECURE(1'b0)         // Security setting for Master 1
    ) axil_interconnect_inst (
        .clk(ap_clk),
        .rst(~ap_rst_n),

        // AXI-Lite Slave Interface
        .s00_axil_awaddr(axi_shared_cfg_AWADDR),
        .s00_axil_awprot(0),
        .s00_axil_awvalid(axi_shared_cfg_AWVALID),
        .s00_axil_awready(axi_shared_cfg_AWREADY),
        .s00_axil_wdata(axi_shared_cfg_WDATA),
        .s00_axil_wstrb(axi_shared_cfg_WSTRB),
        .s00_axil_wvalid(axi_shared_cfg_WVALID),
        .s00_axil_wready(axi_shared_cfg_WREADY),
        .s00_axil_bresp(axi_shared_cfg_BRESP),
        .s00_axil_bvalid(axi_shared_cfg_BVALID),
        .s00_axil_bready(axi_shared_cfg_BREADY),
        .s00_axil_araddr(axi_shared_cfg_ARADDR),
        .s00_axil_arprot(0),
        .s00_axil_arvalid(axi_shared_cfg_ARVALID),
        .s00_axil_arready(axi_shared_cfg_ARREADY),
        .s00_axil_rdata(axi_shared_cfg_RDATA),
        .s00_axil_rresp(axi_shared_cfg_RRESP),
        .s00_axil_rvalid(axi_shared_cfg_RVALID),
        .s00_axil_rready(axi_shared_cfg_RREADY),

        // AXI-Lite Master Interface 0
        .m00_axil_awaddr(axi0_cfg_AWADDR),
        .m00_axil_awprot(),
        .m00_axil_awvalid(axi0_cfg_AWVALID),
        .m00_axil_awready(axi0_cfg_AWREADY),
        .m00_axil_wdata(axi0_cfg_WDATA),
        .m00_axil_wstrb(axi0_cfg_WSTRB),
        .m00_axil_wvalid(axi0_cfg_WVALID),
        .m00_axil_wready(axi0_cfg_WREADY),
        .m00_axil_bresp(axi0_cfg_BRESP),
        .m00_axil_bvalid(axi0_cfg_BVALID),
        .m00_axil_bready(axi0_cfg_BREADY),
        .m00_axil_araddr(axi0_cfg_ARADDR),
        .m00_axil_arprot(),
        .m00_axil_arvalid(axi0_cfg_ARVALID),
        .m00_axil_arready(axi0_cfg_ARREADY),
        .m00_axil_rdata(axi0_cfg_RDATA),
        .m00_axil_rresp(axi0_cfg_RRESP),
        .m00_axil_rvalid(axi0_cfg_RVALID),
        .m00_axil_rready(axi0_cfg_RREADY),

        // AXI-Lite Master Interface 1
        .m01_axil_awaddr(axi1_cfg_AWADDR),
        .m01_axil_awprot(),
        .m01_axil_awvalid(axi1_cfg_AWVALID),
        .m01_axil_awready(axi1_cfg_AWREADY),
        .m01_axil_wdata(axi1_cfg_WDATA),
        .m01_axil_wstrb(axi1_cfg_WSTRB),
        .m01_axil_wvalid(axi1_cfg_WVALID),
        .m01_axil_wready(axi1_cfg_WREADY),
        .m01_axil_bresp(axi1_cfg_BRESP),
        .m01_axil_bvalid(axi1_cfg_BVALID),
        .m01_axil_bready(axi1_cfg_BREADY),
        .m01_axil_araddr(axi1_cfg_ARADDR),
        .m01_axil_arprot(),
        .m01_axil_arvalid(axi1_cfg_ARVALID),
        .m01_axil_arready(axi1_cfg_ARREADY),
        .m01_axil_rdata(axi1_cfg_RDATA),
        .m01_axil_rresp(axi1_cfg_RRESP),
        .m01_axil_rvalid(axi1_cfg_RVALID),
        .m01_axil_rready(axi1_cfg_RREADY)
    );

    axi_axil_adapter #(
        .ADDR_WIDTH(8),
        .AXI_DATA_WIDTH(32),
        .AXI_STRB_WIDTH(4),
        .AXI_ID_WIDTH(8),                 // Slave AXI ID width
        .AXIL_DATA_WIDTH(32),     // Master AXI lite data width
        .AXIL_STRB_WIDTH(4),     // Master AXI lite wstrb width
        .CONVERT_BURST(1),                // Convert full-width burst
        .CONVERT_NARROW_BURST(0)          // Convert narrow burst
    ) axi_axil_adapter_inst (
        .clk(ap_clk),
        .rst(~ap_rst_n),

        // AXI Slave interface
        .s_axi_awid(s_axi_awid),
        .s_axi_awaddr(s_axi_awaddr),
        .s_axi_awlen(s_axi_awlen),
        .s_axi_awsize(s_axi_awsize),
        .s_axi_awburst(s_axi_awburst),
        .s_axi_awlock(s_axi_awlock),
        .s_axi_awcache(s_axi_awcache),
        .s_axi_awprot(s_axi_awprot),
        .s_axi_awvalid(s_axi_awvalid),
        .s_axi_awready(s_axi_awready),
        .s_axi_wdata(s_axi_wdata),
        .s_axi_wstrb(s_axi_wstrb),
        .s_axi_wlast(s_axi_wlast),
        .s_axi_wvalid(s_axi_wvalid),
        .s_axi_wready(s_axi_wready),
        .s_axi_bid(s_axi_bid),
        .s_axi_bresp(s_axi_bresp),
        .s_axi_bvalid(s_axi_bvalid),
        .s_axi_bready(s_axi_bready),
        .s_axi_arid(s_axi_arid),
        .s_axi_araddr(s_axi_araddr),
        .s_axi_arlen(s_axi_arlen),
        .s_axi_arsize(s_axi_arsize),
        .s_axi_arburst(s_axi_arburst),
        .s_axi_arlock(s_axi_arlock),
        .s_axi_arcache(s_axi_arcache),
        .s_axi_arprot(s_axi_arprot),
        .s_axi_arvalid(s_axi_arvalid),
        .s_axi_arready(s_axi_arready),
        .s_axi_rid(s_axi_rid),
        .s_axi_rdata(s_axi_rdata),
        .s_axi_rresp(s_axi_rresp),
        .s_axi_rlast(s_axi_rlast),
        .s_axi_rvalid(s_axi_rvalid),
        .s_axi_rready(s_axi_rready),

        // AXI Lite Master interface
        .m_axil_awaddr(axi_shared_cfg_AWADDR),
        .m_axil_awprot(),
        .m_axil_awvalid(axi_shared_cfg_AWVALID),
        .m_axil_awready(axi_shared_cfg_AWREADY),
        .m_axil_wdata(axi_shared_cfg_WDATA),
        .m_axil_wstrb(axi_shared_cfg_WSTRB),
        .m_axil_wvalid(axi_shared_cfg_WVALID),
        .m_axil_wready(axi_shared_cfg_WREADY),
        .m_axil_bresp(axi_shared_cfg_BRESP),
        .m_axil_bvalid(axi_shared_cfg_BVALID),
        .m_axil_bready(axi_shared_cfg_BREADY),
        .m_axil_araddr(axi_shared_cfg_ARADDR),
        .m_axil_arprot(),
        .m_axil_arvalid(axi_shared_cfg_ARVALID),
        .m_axil_arready(axi_shared_cfg_ARREADY),
        .m_axil_rdata(axi_shared_cfg_RDATA),
        .m_axil_rresp(axi_shared_cfg_RRESP),
        .m_axil_rvalid(axi_shared_cfg_RVALID),
        .m_axil_rready(axi_shared_cfg_RREADY)
    );


endmodule