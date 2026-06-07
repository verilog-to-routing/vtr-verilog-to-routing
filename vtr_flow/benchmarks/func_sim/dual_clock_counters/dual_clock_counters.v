// two counters on two independent clocks
//  - countA increments on each rising edge of clk_a (reset clears it)
//  - countB increments on each rising edge of clk_b (reset clears it)
//  - output is {countB, countA} on single-bit pins o7..o0
//  - clk_b domain is a submodule so yosys/vpr keep both clock pins on top
// clk_b must be a submodule otherwise parmys will optimize the clock out of the post-synthesis design
// see issue https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/3582 for further detail
module clk_b_domain (
    input  clk_b,
    input  rst,
    output o4,
    output o5,
    output o6,
    output o7
);
    reg [3:0] countB;

    always @(posedge clk_b) begin
        if (rst)
            countB <= 4'd0;
        else
            countB <= countB + 4'd1;
    end

    assign {o7, o6, o5, o4} = countB;
endmodule

module top (
    input  clk_a,
    input  clk_b,
    input  rst,
    output o0,
    output o1,
    output o2,
    output o3,
    output o4,
    output o5,
    output o6,
    output o7
);
    reg [3:0] countA;

    always @(posedge clk_a) begin
        if (rst)
            countA <= 4'd0;
        else
            countA <= countA + 4'd1;
    end

    assign {o3, o2, o1, o0} = countA;

    clk_b_domain u_countB (
        .clk_b(clk_b),
        .rst(rst),
        .o4(o4),
        .o5(o5),
        .o6(o6),
        .o7(o7)
    );
endmodule
