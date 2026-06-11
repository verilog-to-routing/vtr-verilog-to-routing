// overlapping 101 sequence detector 
//  - watches data_in on each rising clock edge
//  - moves through states when it sees 1, then 0, then 1
//  - drives match high while in the state that means "101" was just seen
//  - reset forces the state machine back to idle
//  - single-bit ports so post-implementation netlist port names match the testbench
module top (
    input  clk,
    input  rst,
    input  data_in,
    output match
);
    reg [1:0] state;

    localparam [1:0] s_idle = 2'd0;
    localparam [1:0] s_got1  = 2'd1;
    localparam [1:0] s_got10 = 2'd2;
    localparam [1:0] s_got101 = 2'd3;

    always @(posedge clk) begin
        if (rst) begin
            state <= s_idle;
        end else begin
            case (state)
                s_idle:   state <= data_in ? s_got1 : s_idle;
                s_got1:   state <= data_in ? s_got1 : s_got10;
                s_got10:  state <= data_in ? s_got101 : s_idle;
                s_got101: state <= data_in ? s_got1 : s_got10;
                default:  state <= s_idle;
            endcase
        end
    end

    assign match = (state == s_got101);
endmodule
