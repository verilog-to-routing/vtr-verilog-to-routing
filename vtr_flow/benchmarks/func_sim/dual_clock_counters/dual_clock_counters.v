// two counters on two independent clocks
//  - countA increments on each rising edge of clk_a (reset clears it)
//  - countB increments on each rising edge of clk_b (reset clears it)
//  - output is {countB, countA} on single-bit pins o7..o0
//  - single-bit ports so post-implementation netlist port names match the testbench
module top (
    input  clk_a,
    input  clk_b,
    input  rst,
    output o0, o1, o2, o3, o4, o5, o6, o7
);
    reg [3:0] countA;
    reg [3:0] countB;

    always @(posedge clk_a) begin
        if (rst)
            countA <= 4'd0;
        else
            countA <= countA + 4'd1;
    end

    always @(posedge clk_b) begin
        if (rst)
            countB <= 4'd0;
        else
            countB <= countB + 4'd1;
    end

    assign {o7, o6, o5, o4, o3, o2, o1, o0} = {countB, countA};
endmodule
