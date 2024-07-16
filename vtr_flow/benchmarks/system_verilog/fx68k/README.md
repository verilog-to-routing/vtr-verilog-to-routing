# fx68k
FX68K 68000 cycle accurate SystemVerilog core

Copyright (c) 2018 by Jorge Cwik
fx68k@fxatari.com

FX68K is a 68000 cycle exact compatible core. At least in theory, it should be impossible to distinguish functionally from a real 68K processor.

On Cyclone families it uses just over 5,100 LEs and about 5KB internal ram, reaching a max effective clock frequency close to 40MHz. Some optimizations are still possible to implement and increase the performance.

The core is fully synchronous. Considerable effort was made to avoid any asynchronous logic.

Written in SystemVerilog.

The timing of the external bus signals is exactly as the original processor. The only feature that is not implemented yet is bus retry using the external HALT input signal.

It was designed to replace an actual chip on a real board. This wasn't yet tested however and not all necessary output enable control signals are fully implemented.
