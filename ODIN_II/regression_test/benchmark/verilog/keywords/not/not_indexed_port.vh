`define RANGE [1:0]

module simple_op(a,out);
    input 	`RANGE a;
    output 	`RANGE out;

    not(out[0],a[0]);
    not(out[1],a[1]);
endmodule 