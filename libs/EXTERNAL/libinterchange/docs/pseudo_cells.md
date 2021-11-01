# Pseudo Cells

Pseudo PIPs and site pseudo PIPs are edges in the device graph that route
through other sites and/or BELs.  Pseudo cells are the expression of what
routing resources are "blocked" by the use of a pseudo PIP.

The device database currently expresses pseudo PIPs as using BEL pins within
the site that the pseudo PIP is attached too.  Both input and output BEL pins
are included in the pseudo cell definition, but only output BEL pins "block"
the site wire.

### Example

In the case of a Xilinx 7-series `CLBLL`'s `CLBLL_LL_A1` to `CLBLL_LL_A`
pseudo PIP, this PIP connects a signal from an input site port to an output
site port.  Each site wire that is consumed has **both** a output BEL pin
(from the site wire source) and a input BEL pin (connected to either a logic
BEL, e.g. `A6LUT` or routing BEL, e.g. `AUSED` or output site port BEL, e.g.
`A`).

In the case of a `CLBLL`'s `CLBLL_LL_A` to `CLBLL_LL_AMUX` pseudo PIP, this
PIP connects a signal from an output site port to an output site port.  In
this case, it is assumed and required that some of the site wires are already
bound to the relevant net (by virtue of the wire `CLBLL_LL_A` already being
part of the net).  In this case, the first BEL pin will be an input BEL pin
(specifically `AOUTMUX/O6`) that indicates that a site PIP is used as part of
the pseudo PIP.  However in this case, this edge does **not** block the site
wire, instead it only requires it.  The following output BEL pin (specifically
 `AOUTMUX/OUT`) blocks the site wire `AMUX`.

### Future enhancements

Pseudo cells right now only have BEL pins used by the pseudo PIP.  This
ignores the fact that some BEL's when route through may have constraint
implications.  For example, routing through a LUT BEL requires that it be in
LUT mode.  If that BEL is in either a SRL or LUT-RAM mode, the LUT route
through may not operate properly.
