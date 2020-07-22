module simple_op(a,b,c);
    input  a;
    input  b;
    output c;

    and(c,a,b);
	 
	parameter value = 5;
		
    specify 
	    specparam stallup = value;
	    specparam stalldown = stallup + value;
        (a => c) = (stallup);
        (b => c) = (stalldown);
    endspecify 

endmodule 