module down_counter ( clk, out1, rst1, rst2 );
    input rst1, rst2, clk;
    output reg [15:0] out1;

    wire [15:0] a;

    genvar i;
    genvar j;
    generate
        for (i=0; i<4; i=i+1) begin : outer_loop
            for (j=0; j<4; j=j+1) begin : inner_loop
                assign a[(i*4)+j] = 1'b1;
            end
        end
    endgenerate

    always @ (posedge clk)
    begin
        if (rst1 && rst2)
            out1 <= a;
        else
            out1 <= out1 - 1;
    end

endmodule
