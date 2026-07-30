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

// _DFF_N_ covered by dff
(* techmap_celltype = "$_DFF_N_" *)
module tech__DFF_N_
  (
    C,
    D,
    Q
   );

   input C;
   input D;
   input Q;

   dff _TECHMAP_REPLACE_
      (  .clk(!C), .D(D), .Q(Q) );

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

// _DFFE_NP_ covered by dffe
(* techmap_celltype = "$_DFFE_NP_" *)
module tech__DFFE_NP_
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
      (  .clk(!C), .D(D), .E(E), .Q(Q) );

endmodule

// _DFFE_PN_ covered by dffe
(* techmap_celltype = "$_DFFE_PN_" *)
module tech__DFFE_PN_
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
      (  .clk(C), .D(D), .E(!E), .Q(Q) );

endmodule

// _DFFE_NN_ covered by dffe
(* techmap_celltype = "$_DFFE_NN_" *)
module tech__DFFE_NN_
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
      (  .clk(!C), .D(D), .E(!E), .Q(Q) );

endmodule

// _SDFF_PN1_ covered by dffh
(* techmap_celltype = "$_SDFF_PN1_" *)
module tech__SDFF_PN1_
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

   dffh _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .H(R), .Q(Q) );

endmodule

// _SDFF_PP1_ covered by dffh
(* techmap_celltype = "$_SDFF_PP1_" *)
module tech__SDFF_PP1_
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

   dffh _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .H(!R), .Q(Q) );

endmodule

// _SDFF_PN0_ covered by dffl
(* techmap_celltype = "$_SDFF_PN0_" *)
module tech__SDFF_PN0_
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

   dffl _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .L(R), .Q(Q) );

endmodule

// _SDFF_PP0_ covered by dffl
(* techmap_celltype = "$_SDFF_PP0_" *)
module tech__SDFF_PP0_
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

   dffl _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .L(!R), .Q(Q) );

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

// _DFF_NN0_ covered by dffr
(* techmap_celltype = "$_DFF_NN0_" *)
module tech__DFF_NN0_
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
      (  .clk(!C), .D(D), .R(R), .Q(Q) );

endmodule

// _DFF_PP0_ covered by dffr
(* techmap_celltype = "$_DFF_PP0_" *)
module tech__DFF_PP0_
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
      (  .clk(C), .D(D), .R(!R), .Q(Q) );

endmodule

// _DFF_PN1_ covered by dffs
(* techmap_celltype = "$_DFF_PN1_" *)
module tech__DFF_PN1_
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

   dffs _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .S(R), .Q(Q) );

endmodule

// _DFF_PP1_ covered by dffs
(* techmap_celltype = "$_DFF_PP1_" *)
module tech__DFF_PP1_
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

   dffs _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .S(!R), .Q(Q) );

endmodule

// _SDFFE_PN1P_ covered by dffeh
(* techmap_celltype = "$_SDFFE_PN1P_" *)
module tech__SDFFE_PN1P_
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

   dffeh _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(E), .H(R), .Q(Q) );

endmodule

// _SDFFE_PN1N_ covered by dffeh
(* techmap_celltype = "$_SDFFE_PN1N_" *)
module tech__SDFFE_PN1N_
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

   dffeh _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(!E), .H(R), .Q(Q) );

endmodule

// _SDFFE_PP1P_ covered by dffeh
(* techmap_celltype = "$_SDFFE_PP1P_" *)
module tech__SDFFE_PP1P_
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

   dffeh _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(E), .H(!R), .Q(Q) );

endmodule

// _SDFFE_PP1N_ covered by dffeh
(* techmap_celltype = "$_SDFFE_PP1N_" *)
module tech__SDFFE_PP1N_
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

   dffeh _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(!E), .H(!R), .Q(Q) );

endmodule

// _SDFFE_PN0P_ covered by dffel
(* techmap_celltype = "$_SDFFE_PN0P_" *)
module tech__SDFFE_PN0P_
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

   dffel _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(E), .L(R), .Q(Q) );

endmodule

// _SDFFE_PN0N_ covered by dffel
(* techmap_celltype = "$_SDFFE_PN0N_" *)
module tech__SDFFE_PN0N_
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

   dffel _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(!E), .L(R), .Q(Q) );

endmodule


// _SDFFE_PP0P_ covered by dffel
(* techmap_celltype = "$_SDFFE_PP0P_" *)
module tech__SDFFE_PP0P_
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

   dffel _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(E), .L(!R), .Q(Q) );

endmodule

// _SDFFE_PP0N_ covered by dffel
(* techmap_celltype = "$_SDFFE_PP0N_" *)
module tech__SDFFE_PP0N_
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

   dffel _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(!E), .L(!R), .Q(Q) );

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

// _DFFE_NN0P_ covered by dffer
(* techmap_celltype = "$_DFFE_NN0P_" *)
module tech__DFFE_NN0P_
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
      (  .clk(!C), .D(D), .E(E), .R(R), .Q(Q) );

endmodule

// _DFFE_PN0N_ covered by dffer
(* techmap_celltype = "$_DFFE_PN0N_" *)
module tech__DFFE_PN0N_
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
      (  .clk(C), .D(D), .E(!E), .R(R), .Q(Q) );

endmodule

// _DFFE_PN1P_ covered by dffes
(* techmap_celltype = "$_DFFE_PN1P_" *)
module tech__DFFE_PN1P_
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

   dffes _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(E), .R(R), .Q(Q) );

endmodule

// _DFFE_PN1N_ covered by dffes
(* techmap_celltype = "$_DFFE_PN1N_" *)
module tech__DFFE_PN1N_
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

   dffes _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(!E), .R(R), .Q(Q) );

endmodule

// _DFFE_NN1P_ covered by dffes
(* techmap_celltype = "$_DFFE_NN1P_" *)
module tech__DFFE_NN1P_
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

   dffes _TECHMAP_REPLACE_
      (  .clk(!C), .D(D), .E(E), .R(R), .Q(Q) );

endmodule

// _DFFE_PP0P_ covered by dffer
(* techmap_celltype = "$_DFFE_PP0P_" *)
module tech__DFFE_PP0P_
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
      (  .clk(C), .D(D), .E(E), .R(!R), .Q(Q) );

endmodule

// _DFFE_PP0N_ covered by dffer
(* techmap_celltype = "$_DFFE_PP0N_" *)
module tech__DFFE_PP0N_
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
      (  .clk(C), .D(D), .E(!E), .R(!R), .Q(Q) );

endmodule

// _DFFE_PP1P_ covered by dffes
(* techmap_celltype = "$_DFFE_PP1P_" *)
module tech__DFFE_PP1P_
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

   dffes _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(E), .R(!R), .Q(Q) );

endmodule

// _DFFE_PP1N_ covered by dffes
(* techmap_celltype = "$_DFFE_PP1N_" *)
module tech__DFFE_PP1N_
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

   dffes _TECHMAP_REPLACE_
      (  .clk(C), .D(D), .E(!E), .R(!R), .Q(Q) );

endmodule
