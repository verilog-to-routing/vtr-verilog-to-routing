#!/usr/bin/env python3
"""build delay abc scripts from the vendored wildebeest delay_lut6.scr.

writes delay_gia_opt.scr which is the gia delay opts ending as gates
not luts so it can be pass1 before a separate abc -luts map pass.

yosys segfaults when reintegrating gia-mapped lut blif from the stock
delay_lut6.scr even after classic if/mfs2/lutpack. returning gates from
gia and mapping with abc -luts avoids that path.
"""
from pathlib import Path

here = Path(__file__).resolve().parent
repoRoot = here.parents[4]
src = repoRoot / "mosaic/wildebeest/src/abc_scripts/LUT6/BEST/delay_lut6.scr"
text = src.read_text(encoding="utf-8").replace("\r\n", "\n")
text = text.replace("delay_lut6.blif", "delay_scratch.blif")

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

header = """# delay_gia_opt.scr
# this script is wildebeest LUT6/BEST/delay_lut6.scr gia delay opts ending as gates/aig.
# invoke pass1: abc -script <this file>
# then pass2:   abc -luts <cost>
# LF-only ASCII.
#
"""

gateTail = """# after final &load (packed gia), put the mapped network into classic abc.
# move_names restores ys__n* PI/PO from the scratch blif written at script start
# because dress only moves internal names and leaves pi*/po* behind as null Wire*.
# do not -dress because dress does not fix PI/PO and can abort write_blif.
# do not strash here because strash errors on mapped networks ("only for logic networks").
&put
move_names delay_scratch.blif
"""

out = header + text.replace(oldTail, gateTail)
outPath = here / "delay_gia_opt.scr"
outPath.write_bytes(out.encode("ascii", errors="strict"))
print(f"wrote {outPath} ({outPath.stat().st_size} bytes)")

# short classic map script for pass2 or standalone use
mapScr = """# delay_map.scr is the pass2 fracturable lut map (after gia opt) or standalone.
# invoke: abc -luts <cost> -script <this file>
# LF-only ASCII. classic only.
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
mapPath = here / "delay_map.scr"
mapPath.write_bytes(mapScr.encode("ascii"))
print(f"wrote {mapPath} ({mapPath.stat().st_size} bytes)")
