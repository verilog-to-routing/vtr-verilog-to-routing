"""docstring"""
import argparse

parser = argparse.ArgumentParser(description="Process tab file.")
parser.add_argument("filename", help="CSV file to be parsed")
parser.add_argument(
    "-o", "--overwrite", action="store_true", help="overwrites the old file"
)
parser.add_argument(
    "-n", "--name", action="store", help="designate a new name for the file"
)
parser.add_argument(
    "-v", "--verbose", help="print logging messages", action="store_true"
)

args = parser.parse_args()

# handle the nameing of the new file
if args.name is not None:
    new_file = str(args.name + ".csv")
else:
    new_file = "parse_results.csv"

# Handle overwriting the file in case one exists
if args.overwrite:
    CSV = open(new_file, "w")
else:
    CSV = open(new_file, "x")


if args.verbose:
    print("Now Converting from tab deliminations to comma:")


with open(args.filename) as TAB:
    for line in TAB:
        stuff = line.split("\t")
        for i in range(len(stuff) - 1):
            CSV.write(stuff[i] + ",")
        CSV.write("\n")

CSV.close()
