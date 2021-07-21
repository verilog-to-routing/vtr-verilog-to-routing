module specify_test(a, b,q1,q2);
input a, b;
output q1;
output q2;
specify
( a => q1 ) = 6;   
( b => q2 ) = 7;   
endspecify

assign  q1=a | b ;
assign  q2=a & b ;

endmodule