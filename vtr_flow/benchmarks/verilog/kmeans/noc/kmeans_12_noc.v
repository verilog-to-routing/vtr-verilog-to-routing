
module kmeans_12_noc (
    input wire clk,
    input wire resetn,
    output wire [25:0] dummy_o_out
);
    genvar i;

    generate
        for (i = 0; i < 12; i = i + 1) begin : kmeans_instances
            two_kmeans_with_noc two_kmeans_with_noc_inst (
                .clk(clk),
                .resetn(resetn),
                .dummy_o(dummy_o_out[2*i+1:2*i])
            );
        end
    endgenerate

				

	generate
		for (i = 24; i < 26; i = i + 1) begin : ddr_noc_instances
			
				noc_router_adapter ddr_inst (
                .clk(clk),
                .resetn(resetn),
					 
					  .s_axi_awaddr(0),
					  .s_axi_awlen(0),
					  .s_axi_awsize(0),
					  .s_axi_awburst(0),
					  .s_axi_awvalid(0),
					  .s_axi_awready(),
					  .s_axi_wdata(0),
					  .s_axi_wstrb(0),
					  .s_axi_wlast(0),
					  .s_axi_wvalid(0),
					  .s_axi_wready(),
					  .s_axi_bresp(),
					  .s_axi_bvalid(),
					  .s_axi_bready(0),
					  .s_axi_araddr(0),
					  .s_axi_arlen(0),
					  .s_axi_arsize(0),
					  .s_axi_arburst(0),
					  .s_axi_arvalid(0),
					  .s_axi_arready(),
					  .s_axi_rdata(),
					  .s_axi_rresp(),
					  .s_axi_rlast(),
					  .s_axi_rvalid(),
					  .s_axi_rready(0),
					  
					  .m_axi_awaddr(),
					  .m_axi_awvalid(),
					  .m_axi_awready(0),
					  .m_axi_wdata(),
					  .m_axi_wstrb(),
					  .m_axi_wvalid(),
					  .m_axi_wready(0),
					  .m_axi_bresp(0),
					  .m_axi_bvalid(0),
					  .m_axi_bready(),
					  .m_axi_araddr(),
					  .m_axi_arvalid(),
					  .m_axi_arready(0),
					  .m_axi_rdata(0),
					  .m_axi_rresp(0),
					  .m_axi_rvalid(0),
					  .m_axi_rready(dummy_o_out[i]),
            );
		
		end
	endgenerate

endmodule
