import sys

GOOD_CIRCUITS=[
"bgm.v",
"stereovision0.v",
"stereovision1.v",
"LU8PEEng.v",
"mkDelayWorker32B.v"
]

def geomean(l):
    return reduce(lambda x,y: x*y, l) ** (1.0/len(l))

geomean_data = None
with open(sys.argv[1]) as f:
    first = True
    for line in f:
        parts = line.split()
        if parts[0] not in GOOD_CIRCUITS and not first:
            continue
        if first:
            char = " ^ "
            first = False
        else:
            char = " | "



        if not geomean_data:
            geomean_data = {}

        for i in range(len(parts)):
            if i not in geomean_data:
                geomean_data[i] = []
            geomean_data[i].append(parts[i])

        print char[1:] + char.join(parts) + char[0:2]

results = []
for col in sorted(geomean_data.keys()):
    if col == 0: continue

    data = map(lambda x: x.strip(), geomean_data[col][1:])
    data = filter(lambda x: x!="-1", data)
    numerical_data = map(lambda x: float(x), data)
    gm = geomean(numerical_data)
    results.append(str(gm))

results.insert(0, "geomean")
print char[1:] + char.join(results) + char[0:2]
