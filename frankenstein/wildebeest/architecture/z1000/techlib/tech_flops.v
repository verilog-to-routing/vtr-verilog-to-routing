// _DFF_P_ covered by dff
(* techmap_celltype = "$_FF_ $_DFF_P_" *)
module tech__DFF_P_
  (
    C,
    D,
    Q
   );

   input C;
   input D;
   input Q;

   dff _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .Q(Q) );

endmodule

// _DFFE_PP_ covered by dffe
(* techmap_celltype = "$_DFFE_PP_" *)
module tech__DFFE_PP_
  (
    C,
    D,
    E,
    Q
   );

   input C;
   input D;
   input E;
   input Q;

   dffe _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(E), .Q(Q) );

endmodule

// _DFF_PN0_ covered by dffr
(* techmap_celltype = "$_DFF_PN0_" *)
module tech__DFF_PN0_
  (
    C,
    D,
    R,
    Q
   );

   input C;
   input D;
   input R;
   input Q;

   dffr _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .R(R), .Q(Q) );

endmodule

// _DFFE_PN0P_ covered by dffer
(* techmap_celltype = "$_DFFE_PN0P_" *)
module tech__DFFE_PN0P_
  (
    C,
    D,
    E,
    R,
    Q
   );

   input C;
   input D;
   input E;
   input R;
   input Q;

   dffer _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(E), .R(R), .Q(Q) );

endmodule

