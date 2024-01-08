module top(input clock, in, output reg out);
    reg [9:0] register_chain;

	always @(posedge clock) begin
        register_chain[0] <= in;
        register_chain[1] <= register_chain[0];
        register_chain[2] <= register_chain[1] + register_chain[1];
        register_chain[3] <= register_chain[2] + register_chain[2];
        register_chain[4] <= register_chain[3] + register_chain[3];
        register_chain[5] <= register_chain[4];
        register_chain[6] <= register_chain[5];
        register_chain[7] <= register_chain[6];
        register_chain[8] <= register_chain[7];
        register_chain[9] <= register_chain[8];
        out <= register_chain[9];
    end

endmodule
