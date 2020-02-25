
function integer func_out;
input [1:0] x, y;
input rst;
input integer test_int;

case(rst)
    1'b0: func_out = x + y;
    default: func_out = test_int;
endcase


endfunction
