module simple_op(in,out);
    parameter N = 8;
    parameter result = $clog2(N);
    
    input  [result-1:0] in;
    output [result-1:0] out;

    assign out = in;
endmodule