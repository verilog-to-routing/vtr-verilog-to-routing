
module kmeans_12_noc (
    input wire clk,
    input wire resetn,
    output wire [29:0] dummy_o_out
);
    genvar i;

    generate
        for (i = 0; i < 14; i = i + 1) begin : kmeans_instances
            two_kmeans_with_noc two_kmeans_with_noc_inst (
                .clk(clk),
                .resetn(resetn),
                .dummy_o(dummy_o_out[2*i+1:2*i])
            );
        end
    endgenerate


	generate
		for (i = 28; i < 30; i = i + 1) begin : ddr_noc_instances
			
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
                    .s_axi_rvalid(dummy_o_out[i]),
                    .s_axi_rready(0)
                );
		
		end
	endgenerate

endmodule
