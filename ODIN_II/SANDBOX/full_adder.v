module fulladder(
    a,b,cin,cout,out);
input a,b,cin;
output cout,out;
wire [2:0]temp;
xor(temp[0],a,b);//A XZOR B
xor(out,temp[0],cin);// A XOR B XOR CIN
and(temp[1],temp[0],cin);// AXOR B AND CIN
and(temp[2],a,b);
or(cout,temp[1],temp[2]);
endmodule
