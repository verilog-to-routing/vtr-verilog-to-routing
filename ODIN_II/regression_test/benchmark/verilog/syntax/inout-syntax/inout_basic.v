module inout_basic(input dir, inout io, output out);
    assign io = (dir) ? dir : 1'bz;
    assign out = (!dir) ? io : 1'bx;
endmodule