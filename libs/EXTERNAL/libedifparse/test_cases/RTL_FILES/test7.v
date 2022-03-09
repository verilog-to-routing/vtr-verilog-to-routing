module top(a, b, c, d, e, f, out) ;
    input a, b, c, d, e, f ;
    output out ;

    wire tmp1, tmp2 ;

    mid1 i1(a, b, c, d, tmp1) ;
    mid1 i2(a, d, e, f, tmp2) ;
    mid1 i3(tmp1, tmp2, e, f, out) ;

endmodule

module mid1 (mid1_inp1, mid1_inp2, mid1_inp3, mid1_inp4, mid1_out) ;
    input mid1_inp1, mid1_inp2, mid1_inp3, mid1_inp4 ;
    output mid1_out ;
    
    wire tmp1, tmp2 ;

    mid2 m1(mid1_inp1, mid1_inp2, tmp1) ;
    mid2 m2(mid1_inp3, mid1_inp4, tmp2) ;
    mid2 m3(tmp1, tmp2, mid1_out) ;

endmodule

module mid2 (mid2_inp1, mid2_inp2, mid2_out) ;
    input mid2_inp1, mid2_inp2 ;
    output mid2_out ;

    bot b(mid2_inp1, mid2_inp2, mid2_out) ;

endmodule

module bot (bot_inp1, bot_inp2, bot_out) ;
    input bot_inp1, bot_inp2 ;
    output bot_out ;

    assign bot_out = bot_inp1 & bot_inp2 ;

endmodule
