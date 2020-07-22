
module simple_op(in1,in2,in3,out);
    input  in1;
    input  in2;
    input  in3;
    output out;

    `define second

    `ifdef first
        assign out = in1;
    `elsif second
        assign out = in2;
    `else
        assign out = in3;
    `endif 
    
endmodule