module top_module
(
    
    input [1:0] a1, b1, a2, b2,
    input cin1, cin2,
    output [1:0] sumout1, sumout2,
    output cout1, cout2
);

    adder a1 (.a(a1), .b(b1), .cin(cin1), .sumout(sumout1), .cout(cout1));
    adder a2 (a2, b2, cin2, sumout2, cout2);

endmodule