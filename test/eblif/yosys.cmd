read_verilog /project/trees/vtr/vtr_flow/benchmarks/verilog/sha.v
#setattr -set "myattr" "myattrvalue" *
#setparam -set "myparam" "myparamvalue" *
#hierarchy
#proc; opt; techmap; opt;
proc;
write_blif -conn -attr -param -cname sha.blif
