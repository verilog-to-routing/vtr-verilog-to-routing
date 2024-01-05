module simple_op(a,b,c,d);

	input  [7:0] a,b;
	output [7:0] c,d;
	
	always begin
	flip_all(a,c);
	flip_all(b,d);
	end
	
	task flip_all;
        input  [7:0] in;
        output [7:0] out;
        
        begin 
            flip_one(in[7],out[0]);
            flip_one(in[6],out[1]);
            flip_one(in[5],out[2]);
            flip_one(in[4],out[3]);
            flip_one(in[3],out[4]);
            flip_one(in[2],out[5]);
            flip_one(in[1],out[6]);
            flip_one(in[0],out[7]);
        end 
	endtask
	
	task flip_one;
        input in;
        output out;
        
        begin 
            out = in;
        end
	endtask 
	
endmodule 
	