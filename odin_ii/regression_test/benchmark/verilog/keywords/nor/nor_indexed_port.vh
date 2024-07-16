`define RANGE [1:0]

module simple_op(a,b,out);
    input 	`RANGE a;
    input   `RANGE b;
    output 	`RANGE out;

    nor(out[0],a[0],b[0]);
    nor(out[1],a[1],b[1]);
endmodule 