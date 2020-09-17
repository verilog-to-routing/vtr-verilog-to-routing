#!/usr/bin/env python3

############################### blif_splicer.py ################################
#
# Generate the k largest circuits (by total # of logic blocks or occurrences
# of '.names') which can be formed by concatenating n of the MCNC 20 circuits.
# Gives an error for circuits with multiple .model sections.
#
# Author: Michael Wainberg

################################### Settings ###################################

k = 10
n = 2
blifdir = "../benchmarks/blif"

################################################################################

import os
import sys
import glob
import itertools

os.chdir(blifdir)

# Get the name of each circuit (without the '.blif') and its # of logic blocks

circuits = []
for filename in glob.glob("*.blif"):
    if "+" in filename:
        continue  # don't use outputs from previous script runs as inputs
    with open(filename) as infile:
        name = os.path.splitext(filename)[0]  # remove '.blif'
        circuits.append((name, infile.read().count(".names")))

# Iterate over the first k combinations of n circuits in decreasing order,
# sorted by the total number of logic blocks, and stitch them together.
# Note: this could be optimized further by storing the processed version
# of each circuit after it is read in for the first time, since the largest
# circuits will likely be reused often.

nonNetChars = "0123456789\\-\n"  # a valid net name can't use only these

for tupleIndex, nTuple in enumerate(
    sorted(
        itertools.combinations(circuits, n),
        key=lambda item: sum(circuit[1] for circuit in item),
        reverse=True,
    )
):

    # Parse each input blif into the following strings, which represent
    # different sections of the output file.

    sections = [".inputs ", ".outputs ", ""]

    for circuitIndex, circuit in enumerate(nTuple):
        name = circuit[0]
        sectionNumber = 0  # 0 = .inputs, 1 = .outputs, 2 = body
        modelFlag = False
        with open(name + ".blif") as infile:
            # After each circuit, replace the last newline of .inputs and
            # .outputs lines with a line continuation '\' then a newline.
            if circuitIndex > 0:
                sections[0] = sections[0][:-1] + " \\\n"
                sections[1] = sections[1][:-1] + " \\\n"
            for line in infile:
                if not line or line[0] == "\n":
                    continue  # empty line
                if line[0] == ".":
                    if line[1] == "m":  # .model
                        if modelFlag:
                            sys.exit("Multiple .model sections in circuit " + name + ": aborting")
                        modelFlag = True
                        continue
                    if line[1] == "e":  # .end
                        continue
                    split = line.split(" ", 1)
                    if line[1] == "o":  # .outputs
                        sectionNumber = 1
                    elif line[1] != "i":  # .inputs
                        sectionNumber = 2
                        sections[2] += split[0] + " "  # copy 1st word
                    line = split[1]  # skip 1st word

                # Add this line to the appropriate section, after mangling
                # each net name (one which does not contain only nonNetChars
                # and is not 're') with the name of the circuit.

                sections[sectionNumber] += " ".join(
                    name + "_" + word
                    if word != "re" and any(char not in nonNetChars for char in word)
                    else word
                    for word in line.split(" ")
                )

    # Write the concatenation of all sections to the output file.
    # The format of the output filename is 'file1+file2+...+fileN.blif'

    filename = "+".join(circuit[0] for circuit in nTuple) + ".blif"
    with open(filename, "wb") as outfile:
        # Requires the addition of .encode() below to convert
        # string to bytes (avoids TypeError)
        outfile.write(".model top\n".encode() + "".join(sections).encode() + ".end\n".encode())
    print("Created " + filename + "...")

    if tupleIndex == k - 1:
        break
