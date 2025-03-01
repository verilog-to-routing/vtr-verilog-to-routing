
module two_kmeans (
    input  wire                        clk,
    input  wire                        rst,
	 
    output wire [1:0]						dummy_o,

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

    wire   ap_done0;
    wire   ap_idle0;
    wire   ap_ready0;

    wire   ap_done1;
    wire   ap_idle1;
    wire   ap_ready1;

    assign dummy_o[0] = ap_done0 ^ ap_idle0 ^ ap_ready0;
    assign dummy_o[1] = ap_done1 ^ ap_idle1 ^ ap_ready1;
    
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

    kmeans_flat kmeans_inst0 (
        .ap_clk(ap_clk),
        .ap_rst_n(ap_rst_n),

        .ap_start(1),
        .ap_done(ap_done0),
        .ap_idle(ap_idle0),
        .ap_ready(ap_ready0),
        
        .n(10000000),
        .k(100),
        .control(7),
        .buf_ptr_node_x_coords('h0000000000000000),
        .buf_ptr_node_y_coords('h1000000000000000),
        .buf_ptr_node_cluster_assignments('h2000000000000000),
        .buf_ptr_centroid_x_coords('h3000000000000000),
        .buf_ptr_centroid_y_coords('h4000000000000000),
        .buf_ptr_intermediate_cluster_assignments('h5000000000000000),
        .buf_ptr_intermediate_centroid_x_coords('h6000000000000000),
        .buf_ptr_intermediate_centroid_y_coords('h7000000000000000),
        .max_iterations(60000),
        .sub_iterations(600),

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
    );

    kmeans_flat kmeans_inst1 (
        .ap_clk(ap_clk),
        .ap_rst_n(ap_rst_n),

        .ap_start(1),
        .ap_done(ap_done1),
        .ap_idle(ap_idle1),
        .ap_ready(ap_ready1),

        .n(10000000),
        .k(100),
        .control(7),
        .buf_ptr_node_x_coords('h0000000000000000),
        .buf_ptr_node_y_coords('h1000000000000000),
        .buf_ptr_node_cluster_assignments('h2000000000000000),
        .buf_ptr_centroid_x_coords('h3000000000000000),
        .buf_ptr_centroid_y_coords('h4000000000000000),
        .buf_ptr_intermediate_cluster_assignments('h5000000000000000),
        .buf_ptr_intermediate_centroid_x_coords('h6000000000000000),
        .buf_ptr_intermediate_centroid_y_coords('h7000000000000000),
        .max_iterations(60000),
        .sub_iterations(600),

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


endmodule