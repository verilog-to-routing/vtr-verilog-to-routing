 
module simple_op(out1,out2,out3);

    output reg [2:0] out1;
    output reg [2:0] out2;
    output reg [2:0] out3;

    initial begin
            out1 = 3'b101;
        #20 out2 = 3'b011;
        #30 out3 = 3'b010;
    end
endmodule 