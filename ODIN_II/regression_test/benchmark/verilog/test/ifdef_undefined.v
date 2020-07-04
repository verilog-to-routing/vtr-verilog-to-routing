 
module simple_op(in,out);
    $display("Warning::OUTPUT_BLIF want to see if this will print json \n");
    input in;
    output out;

    `define firsts

    `ifdef first
        assign out = in;
    `endif
endmodule 