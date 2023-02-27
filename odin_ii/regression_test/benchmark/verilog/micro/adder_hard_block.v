`define WIDTH 2
module top_module
(
    
    input [`WIDTH-1:0] a, b,
    input cin,
    output [`WIDTH-1:0] sumout,
    output cout
);

	
    adder #(`WIDTH) a1 (.a(a), .b(b), .cin(cin), .sumout(sumout), .cout(cout));

endmodule
