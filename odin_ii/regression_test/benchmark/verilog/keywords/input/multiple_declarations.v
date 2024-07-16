module simple_op(a,b,c,d,e,f,g,h,i,j,k,l,m,n);

    input wire signed a,b,c;
    input unsigned [2:0] d,e;
    input f,g;

    output wire signed h,i,j;
    output unsigned [2:0] k,l;
    output m,n;

    assign h = a;
    assign i = b;
    assign j = c;
    assign k = d;
    assign l = e;
    assign m = f;
    assign n = g;

endmodule 