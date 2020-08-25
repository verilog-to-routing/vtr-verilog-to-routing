`define RANGE [1:0]

module simple_op(a,b,out);
    input 	`RANGE a;
    input   `RANGE b;
    output 	`RANGE out;

    bufif0(out[0],a[0],b[0]);
    bufif0(out[1],a[1],b[1]);
endmodule 