#!/usr/bin/env python3
"""build k6 delay abc scripts from wildebeest BEST/delay_lut6.scr.

writes k6_delay_gia_opt.scr which is the gia delay opts ending as gates
not luts so it can be pass1 before a separate abc -luts map pass.

yosys segfaults when reintegrating gia-mapped lut blif from the stock
delay_lut6.scr even after classic if/mfs2/lutpack. returning gates from
gia and mapping with abc -luts avoids that path.
"""
from pathlib import Path

here = Path(__file__).resolve().parent
repo = here.parents[2]
src = repo / "wildebeest-main/src/abc_scripts/LUT6/BEST/delay_lut6.scr"
text = src.read_text(encoding="utf-8").replace("\r\n", "\n")
text = text.replace("delay_lut6.blif", "k6_delay_scratch.blif")

oldTail = """echo " "
echo "lutpack -S 1;"
&put; lutpack -S 1; &get -n -m; &ps; &save; time

&load;

echo " "
echo "** ABC DELAY LUT6 optimization done !!!"
&put

time
"""

if oldTail not in text:
    raise SystemExit("expected lutpack tail not found in delay_lut6.scr")

header = """# k6_delay_gia_opt.scr
# gia delay opts from wildebeest LUT6/BEST/delay_lut6.scr ending as gates.
# pass1 abc -script this file then pass2 abc -luts
# LF-only ASCII.
#
"""

gateTail = """# move_names restores the ys__n* PI/PO names from the scratch blif.
# dress only moves internal names and can abort write_blif. do not strash
# here either  mapped networks reject it.
&put
move_names k6_delay_scratch.blif
"""

out = header + text.replace(oldTail, gateTail)
outPath = here / "k6_delay_gia_opt.scr"
outPath.write_bytes(out.encode("ascii", errors="strict"))
print(f"wrote {outPath} ({outPath.stat().st_size} bytes)")

# short classic map script for pass2 or standalone use
mapScr = """# k6_delay.scr
# pass2 lut map after the gia opt or a standalone classic map.
# invoke with abc -luts <cost> -script this file. LF-only ASCII.

strash
ifraig
dc2
dch -f
dc2
dch -f
dc2
dch -f
if
mfs2
mfs2
lutpack
"""
mapPath = here / "k6_delay.scr"
mapPath.write_bytes(mapScr.encode("ascii"))
print(f"wrote {mapPath} ({mapPath.stat().st_size} bytes)")
