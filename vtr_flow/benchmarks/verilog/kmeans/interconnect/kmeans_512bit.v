
module kmeans_512bit (
    input  wire                        clk,
    input  wire                        rst,

    /*
     * AXI slave interface
     */

    input  wire             s_axi_awvalid,
    output wire             s_axi_awready,
    input  wire [7:0]       s_axi_awaddr,

    input  wire             s_axi_wvalid,
    output wire             s_axi_wready,
    input  wire [31:0]      s_axi_wdata,
    input  wire [3:0]       s_axi_wstrb,

    input  wire             s_axi_arvalid,
    output wire             s_axi_arready,
    input  wire [7:0]       s_axi_araddr,

    output wire             s_axi_rvalid,
    input  wire             s_axi_rready,
    output wire [31:0]      s_axi_rdata,
    output wire [1:0]       s_axi_rresp,

    output wire             s_axi_bvalid,
    input  wire             s_axi_bready,
    output wire [1:0]       s_axi_bresp,


    /*
     * AXI master interface
     */
    output wire [0:0]      m_axi_awid,
    output wire [63:0]    m_axi_awaddr,
    output wire [7:0]               m_axi_awlen,
    output wire [2:0]               m_axi_awsize,
    output wire [1:0]               m_axi_awburst,
    output wire                     m_axi_awlock,
    output wire [3:0]               m_axi_awcache,
    output wire [2:0]               m_axi_awprot,
    output wire [3:0]               m_axi_awqos,
    output wire [3:0]               m_axi_awregion,
    output wire [0:0]  m_axi_awuser,
    output wire                     m_axi_awvalid,
    input  wire                     m_axi_awready,
    output wire [511:0]  m_axi_wdata,
    output wire [63:0]  m_axi_wstrb,
    output wire                     m_axi_wlast,
    output wire [0:0]   m_axi_wuser,
    output wire                     m_axi_wvalid,
    input  wire                     m_axi_wready,
    input  wire [0:0]      m_axi_bid,
    input  wire [1:0]               m_axi_bresp,
    input  wire [0:0]   m_axi_buser,
    input  wire                     m_axi_bvalid,
    output wire                     m_axi_bready,
    output wire [0:0]      m_axi_arid,
    output wire [63:0]    m_axi_araddr,
    output wire [7:0]               m_axi_arlen,
    output wire [2:0]               m_axi_arsize,
    output wire [1:0]               m_axi_arburst,
    output wire                     m_axi_arlock,
    output wire [3:0]               m_axi_arcache,
    output wire [2:0]               m_axi_arprot,
    output wire [3:0]               m_axi_arqos,
    output wire [3:0]               m_axi_arregion,
    output wire [0:0]  m_axi_aruser,
    output wire                     m_axi_arvalid,
    input  wire                     m_axi_arready,
    input  wire [0:0]      m_axi_rid,
    input  wire [511:0]  m_axi_rdata,
    input  wire [1:0]               m_axi_rresp,
    input  wire                     m_axi_rlast,
    input  wire [0:0]   m_axi_ruser,
    input  wire                     m_axi_rvalid,
    output wire                     m_axi_rready

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

        .s_axi_cfg_AWVALID(s_axi_awvalid),
        .s_axi_cfg_AWREADY(s_axi_awready),
        .s_axi_cfg_AWADDR(s_axi_awaddr),
        .s_axi_cfg_WVALID(s_axi_wvalid),
        .s_axi_cfg_WREADY(s_axi_wready),
        .s_axi_cfg_WDATA(s_axi_wdata),
        .s_axi_cfg_WSTRB(s_axi_wstrb),
        .s_axi_cfg_ARVALID(s_axi_arvalid),
        .s_axi_cfg_ARREADY(s_axi_arready),
        .s_axi_cfg_ARADDR(s_axi_araddr),
        .s_axi_cfg_RVALID(s_axi_rvalid),
        .s_axi_cfg_RREADY(s_axi_rready),
        .s_axi_cfg_RDATA(s_axi_rdata),
        .s_axi_cfg_RRESP(s_axi_rresp),
        .s_axi_cfg_BVALID(s_axi_bvalid),
        .s_axi_cfg_BREADY(s_axi_bready),
        .s_axi_cfg_BRESP(s_axi_bresp),
        .interrupt(interrupt0)
    );


    axi_adapter #
    (
        .ADDR_WIDTH(64),
        .S_DATA_WIDTH(64),
        .S_STRB_WIDTH(8),

        .M_DATA_WIDTH(512),
        .M_STRB_WIDTH(64),
        .ID_WIDTH(1),
        // When adapting to a wider bus, re-pack full-width burst instead of passing through narrow burst if possible
        .CONVERT_BURST(1),
        // When adapting to a wider bus, re-pack all bursts instead of passing through narrow burst if possible
        .CONVERT_NARROW_BURST(0),
    )
    axi_width_adapter_inst
    (
        .clk(clk),
        .rst(rst),

        .s_axi_awid(axi0_mem_AWID),
        .s_axi_awaddr(axi0_mem_AWADDR),
        .s_axi_awlen(axi0_mem_AWLEN),
        .s_axi_awsize(axi0_mem_AWSIZE),
        .s_axi_awburst(axi0_mem_AWBURST),
        .s_axi_awlock(axi0_mem_AWLOCK),
        .s_axi_awcache(axi0_mem_AWCACHE),
        .s_axi_awprot(axi0_mem_AWPROT),
        .s_axi_awqos(axi0_mem_AWQOS),
        .s_axi_awregion(axi0_mem_AWREGION),
        .s_axi_awuser(axi0_mem_AWUSER),
        .s_axi_awvalid(axi0_mem_AWVALID),
        .s_axi_awready(axi0_mem_AWREADY),

        .s_axi_wdata(axi0_mem_WDATA),
        .s_axi_wstrb(axi0_mem_WSTRB),
        .s_axi_wlast(axi0_mem_WLAST),
        .s_axi_wuser(axi0_mem_WUSER),
        .s_axi_wvalid(axi0_mem_WVALID),
        .s_axi_wready(axi0_mem_WREADY),

        .s_axi_bid(axi0_mem_BID),
        .s_axi_bresp(axi0_mem_BRESP),
        .s_axi_buser(axi0_mem_BUSER),
        .s_axi_bvalid(axi0_mem_BVALID),
        .s_axi_bready(axi0_mem_BREADY),

        .s_axi_arid(axi0_mem_ARID),
        .s_axi_araddr(axi0_mem_ARADDR),
        .s_axi_arlen(axi0_mem_ARLEN),
        .s_axi_arsize(axi0_mem_ARSIZE),
        .s_axi_arburst(axi0_mem_ARBURST),
        .s_axi_arlock(axi0_mem_ARLOCK),
        .s_axi_arcache(axi0_mem_ARCACHE),
        .s_axi_arprot(axi0_mem_ARPROT),
        .s_axi_arqos(axi0_mem_ARQOS),
        .s_axi_arregion(axi0_mem_ARREGION),
        .s_axi_aruser(axi0_mem_ARUSER),
        .s_axi_arvalid(axi0_mem_ARVALID),
        .s_axi_arready(axi0_mem_ARREADY),

        .s_axi_rid(axi0_mem_RID),
        .s_axi_rdata(axi0_mem_RDATA),
        .s_axi_rresp(axi0_mem_RRESP),
        .s_axi_rlast(axi0_mem_RLAST),
        .s_axi_ruser(axi0_mem_RUSER),
        .s_axi_rvalid(axi0_mem_RVALID),
        .s_axi_rready(axi0_mem_RREADY),

        /*
         * AXI master interface
         */
        .m_axi_awid(m_axi_awid),
        .m_axi_awaddr(m_axi_awaddr),
        .m_axi_awlen(m_axi_awlen),
        .m_axi_awsize(m_axi_awsize),
        .m_axi_awburst(m_axi_awburst),
        .m_axi_awlock(m_axi_awlock),
        .m_axi_awcache(m_axi_awcache),
        .m_axi_awprot(m_axi_awprot),
        .m_axi_awqos(m_axi_awqos),
        .m_axi_awregion(m_axi_awregion),
        .m_axi_awuser(m_axi_awuser),
        .m_axi_awvalid(m_axi_awvalid),
        .m_axi_awready(m_axi_awready),
        .m_axi_wdata(m_axi_wdata),
        .m_axi_wstrb(m_axi_wstrb),
        .m_axi_wlast(m_axi_wlast),
        .m_axi_wuser(m_axi_wuser),
        .m_axi_wvalid(m_axi_wvalid),
        .m_axi_wready(m_axi_wready),
        .m_axi_bid(m_axi_bid),
        .m_axi_bresp(m_axi_bresp),
        .m_axi_buser(m_axi_buser),
        .m_axi_bvalid(m_axi_bvalid),
        .m_axi_bready(m_axi_bready),
        .m_axi_arid(m_axi_arid),
        .m_axi_araddr(m_axi_araddr),
        .m_axi_arlen(m_axi_arlen),
        .m_axi_arsize(m_axi_arsize),
        .m_axi_arburst(m_axi_arburst),
        .m_axi_arlock(m_axi_arlock),
        .m_axi_arcache(m_axi_arcache),
        .m_axi_arprot(m_axi_arprot),
        .m_axi_arqos(m_axi_arqos),
        .m_axi_arregion(m_axi_arregion),
        .m_axi_aruser(m_axi_aruser),
        .m_axi_arvalid(m_axi_arvalid),
        .m_axi_arready(m_axi_arready),
        .m_axi_rid(m_axi_rid),
        .m_axi_rdata(m_axi_rdata),
        .m_axi_rresp(m_axi_rresp),
        .m_axi_rlast(m_axi_rlast),
        .m_axi_ruser(m_axi_ruser),
        .m_axi_rvalid(m_axi_rvalid),
        .m_axi_rready(m_axi_rready)
    );



endmodule