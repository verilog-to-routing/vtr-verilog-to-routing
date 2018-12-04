import sys


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def make_output_vector(power):
    if power < 0 or int(power) != power:
        eprint("Bad value for exponent: ", power)
    print("c")
    next_a = 0
    for i in range(0, 512):
        value = next_a ** power
        output = f"{value:#0{4}x}"
        print(output[0:2] + output[-2:])
        if i%2 == 1:
            next_a += 1
    value = next_a ** power
    output = f"{value:#0{4}x}"
    print(output[0:2] + output[-2:])
    print("0x00")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: " + sys.argv[0] + " <exponent>")
    power = int(sys.argv[1])
    make_output_vector(power)
