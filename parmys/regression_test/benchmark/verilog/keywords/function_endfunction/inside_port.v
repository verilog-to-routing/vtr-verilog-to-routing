module simple_op(a,b,c,d);

    input  [7:0] a,b;
    output [7:0] c,d;

    assign c = flip(a);
    assign d = flip(b);

    function [7:0] flip(input [7:0] in);
    begin
        flip[0] = in [7];
        flip[1] = in [6];
        flip[2] = in [5];
        flip[3] = in [4];
        flip[4] = in [3];
        flip[5] = in [2];
        flip[6] = in [1];
        flip[7] = in [0];
    end
    endfunction
endmodule 
	