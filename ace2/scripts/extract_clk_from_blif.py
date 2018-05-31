#!/usr/bin/env python
import sys
import re


with open(sys.argv[2], 'r') as fp:
  text = fp.read()

# print(text)


matches = re.findall("^\s*\.latch\s+.*?\s+.*?\s+.*?\s+(.*?)\s+.*?$", text, re.M | re.I)

clks = list(set(matches))
# '.latch n906 top.boundcontroller+boundcont01^addrind~8_FF_NODE re top^tm3_clk_v0 0'

print("Clocks found:")
for clk in clks:
	print("  ", clk)

if len(clks) == 1:
	with open(sys.argv[1], 'w+') as fp:
		fp.write(clks[0])

elif len(clks) > 1:
	print("ACE2 activity estimation does not support circuits with multiple clocks")
	sys.exit(-1)
elif len(clks) == 0:
	print("ACE2 activity estimation does not support asynchronous circuits")
	sys.exit(-1)
