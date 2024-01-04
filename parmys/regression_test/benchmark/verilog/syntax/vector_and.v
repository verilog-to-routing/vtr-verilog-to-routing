module vec_and (in1,in2,out);
input  [1:0] in1,in2;
output [1:0] out;

and(out[0], in1[0],in2[0]);
and(out[1], in1[1],in2[1]);
endmodule