module simple_op(a,b,c,d);

	input  [7:0] a,b;
	output [7:0] c,d;
	
	always begin
	flip(a,c);
	flip(b,d);
	end
	
	task flip(input  [7:0] in,output [7:0] out);
	
	begin 
	
	out[0] = in[7];
	out[1] = in[6];
	out[2] = in[5];
	out[3] = in[4];
	out[4] = in[3];
	out[5] = in[2];
	out[6] = in[1];
	out[7] = in[0];
	
	end
	
	endtask
	
	endmodule 