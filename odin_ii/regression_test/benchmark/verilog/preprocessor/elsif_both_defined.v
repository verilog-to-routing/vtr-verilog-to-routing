
module simple_op(in1,in2,in3,out);
    input  in1;
    input  in2;
    input  in3;
    output out;

    $display("Expect:: ELSIF this test is expected to pass, if it fails the `elsif are not functioning properly.\n");

    `define first
    `define second

    `ifdef first
        assign out = in1;
    `elsif second
        assign out = in2;
    `else
        assign out = in3;
    `endif 

endmodule