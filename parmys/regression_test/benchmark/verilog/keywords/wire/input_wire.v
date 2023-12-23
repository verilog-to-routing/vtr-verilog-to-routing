module simple_op(a,b,a_wire,b_wire);
    input wire a;
    input wire [3:0] a_wire;
    
    output wire b;
    output wire [3:0] b_wire;

    assign b = a;
    assign b_wire = a_wire;

endmodule 