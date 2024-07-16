
module simple_op(out);
    output [2:0] out;

    $display("Expect::NESTED_IFDEF this test is expected to pass, if it fails the `ifdefs are not functioning properly.\n");

`ifdef nest_one
    `ifdef nest_two
        `ifdef nest_three
            assign out = 3'b111;
        `else 
            assign out = 3'b110;
        `endif
    `else
        `ifdef nest_three
            assign out = 3'b101;
        `else 
            assign out = 3'b100;
        `endif
    `endif
`else 
    `ifdef nest_two
        `ifdef nest_three
            assign out = 3'b011;
        `else 
            assign out = 3'b010;
        `endif
    `else
        `ifdef nest_three
            assign out = 3'b001;
        `else 
            assign out = 3'b000;
        `endif
    `endif
`endif

endmodule 