`timescale 1ns / 1ps

module test(
    input A,
    input B,
    output Z2
    );

and (Z2,A,B);
specify 
specparam h=5;
endspecify
endmodule
