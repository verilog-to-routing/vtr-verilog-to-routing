module simple_op(a,b,c);
    input  a;
    input  b;
    output c;

    and(c,a,b);
	 
	specparam stallup = 5;
	specparam stalldown = 6;
		
    specify 
        (a => c) = (stallup,stalldown);
        (b => c) = (stallup,stalldown);
    endspecify

endmodule 