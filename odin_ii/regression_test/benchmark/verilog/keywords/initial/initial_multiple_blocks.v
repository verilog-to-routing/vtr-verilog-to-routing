module simple_op(in1,in2,out1,out2,out3,out4,out5);
    input [2:0] in1;
    input [2:0] in2;
    output reg [2:0] out1;
    output reg [2:0] out2;
    output reg [2:0] out3;
    output reg [2:0] out4;
    output reg [2:0] out5;

    initial
        out1 = in1;

    initial begin
            out2 = in2;
        #10 out3 = 3'b001;
    end

    initial begin
        #20 out4 = 3'b100;
        #30 out5 = 3'b101;
    end
    
endmodule