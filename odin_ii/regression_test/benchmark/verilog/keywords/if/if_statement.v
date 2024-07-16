module simple_op(in,en,out);
    
    input in;
    input en;
    output reg out;
always @(*) begin

    if(en)
        out = in; 
end
endmodule 