module And2in (A,B,C1,C2);
 
input [7:0] A  ;
input  [7:0] B ;
output [3:0] C1 ;
output [3:0] C2 ;

assign  C1=A[0+:4] & B[0+:4] ;
assign C2=A[3+:5] & B[3+:5] ;

endmodule
