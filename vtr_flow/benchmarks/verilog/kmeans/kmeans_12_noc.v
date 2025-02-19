
module kmeans_12_noc (
    input wire clk,
    input wire resetn,
    output wire [13:0] dummy_o_out
);
    genvar i; // Declare a generate variable

    generate
        for (i = 0; i < 12; i = i + 1) begin : kmeans_instances
            two_kmeans_with_noc two_kmeans_with_noc_inst (
                .clk(clk),
                .resetn(resetn),
                .dummy_o(dummy_o_out[i])
            );
        end
    endgenerate

    generate
        for (i = 12; i < 14; i = i + 1) begin : ddr_noc_instances
            two_kmeans_with_noc two_kmeans_with_noc_inst (
                .clk(clk),
                .resetn(resetn),
                .dummy_o(dummy_o_out[i])
            );
        end
    endgenerate

endmodule
