module simple_op(in,out);

    input  [7:0] in;
    output [7:0] out;

    assign out = flip(in);

    function [7:0] flip;
        input [7:0] in;
        begin
			flip_one(in[0],flip[7]);
			flip_one(in[1],flip[6]);
			flip_one(in[2],flip[5]);
			flip_one(in[3],flip[4]);
			flip_one(in[4],flip[3]);
			flip_one(in[5],flip[2]);
			flip_one(in[6],flip[1]);
			flip_one(in[7],flip[0]);
        end
    endfunction

	task flip_one;
			  input in;
			  output out;
			  
			  begin 
					out = in;
			  end
	endtask 
endmodule 