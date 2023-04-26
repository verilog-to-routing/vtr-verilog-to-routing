module vec_buf (in,out);
input  [1:0] in;
output [1:0] out;

buf(out[0], in[0]);
buf(out[1], in[1]);
endmodule