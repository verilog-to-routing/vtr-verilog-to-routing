module And2in (A,B,C1,C2);

input   [5:0] A  ;
input   [5:0] B ;
output  [2:0] C1 ;
output  [2:0] C2 ;

assign  C1 = A[0+:3] & B[0+:3] ;
assign  C2 = A[3+:3] & B[3+:3] ;

endmodule
