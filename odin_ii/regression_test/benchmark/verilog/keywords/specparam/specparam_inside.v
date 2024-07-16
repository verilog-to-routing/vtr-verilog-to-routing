module simple_op(a,b,c);
    input  a;
    input  b;
    output c;

    and(c,a,b);
		
    specify 
		specparam stallup = 5;
	   specparam stalldown = 6;
       (a => c) = (stallup,stalldown);
       (b => c) = (stallup,stalldown);
    endspecify

endmodule 