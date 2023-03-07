specparam stallup = 5;
specparam stalldown = 6;

module simple_op(a,b,c);
    input  a;
    input  b;
    output c;

    and(c,a,b);
		
    specify 
       (a => c) = (stallup,stalldown);
       (b => c) = (stallup,stalldown);
    endspecify

endmodule 