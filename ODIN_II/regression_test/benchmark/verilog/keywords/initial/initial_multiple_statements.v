module simple_op(in,out1,out2);
    input in;
    output reg [7:0] out1;
    output reg out2;

    initial begin
        out1 = 8'b10101010;
        out2 = in;
    end 

endmodule