module top_module
(
    
    input [1:0] a, b,
    input cin,
    output [1:0] sumout,
    output cout
);

    adder a1 (.a(a), .b(b), .cin(cin), .sumout(sumout), .cout(cout));

endmodule
