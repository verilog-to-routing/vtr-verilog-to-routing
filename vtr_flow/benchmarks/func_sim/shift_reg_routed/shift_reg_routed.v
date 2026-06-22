// 8-bit shift register
//  - main goal here is to test routed clock nets
//  - on each rising clock edge, shifts stored bits up by one
//  - shift_in becomes the new low bit
//  - reset clears the register to zero
//  - single-bit ports so post-implementation netlist port names match the testbench
module top (
    input  clk,
    input  rst,
    input  shift_in,
    output q0, q1, q2, q3, q4, q5, q6, q7
);
    reg [7:0] shreg;

    assign {q7, q6, q5, q4, q3, q2, q1, q0} = shreg;

    always @(posedge clk) begin
        if (rst)
            shreg <= 8'h00;
        else
            shreg <= {shreg[6:0], shift_in};
    end
endmodule
