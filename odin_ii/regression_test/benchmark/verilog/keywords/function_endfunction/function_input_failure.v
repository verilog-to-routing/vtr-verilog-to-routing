module simple_op(in,out);

    input  [7:0] in;
    output [7:0] out;

    $display("Expect::FUNCTION Test should fail, need at least 1 input");

    assign out = add();

    function [7:0] add;
    begin
	 add = 4+7;
    end
    endfunction
endmodule 
	
