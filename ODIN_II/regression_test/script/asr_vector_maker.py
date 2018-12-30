import sys


def make_output_file(filename, vector):
    with open(filename, 'w') as out_file:
        out_file.write("c\n")
        for output in vector:
            out_file.write(hex(output))
            out_file.write("\n")
    return True


def make_output_vector(bits, length, inputs):
    output = 0
    outputs = []
    height = len(inputs[0])
    shift_flag = 1
    for i in range(0, height):
        if i%2 == shift_flag:
            if inputs[2][i] != 1:
                output = shift(bits, length, inputs[0][i])
            else:
                output = 0
        outputs.append(output)
    return outputs


def shift(bits, length, value):
    new_value = value >> length
    msb_mask = (1 << (bits - 1))
    if (value & msb_mask) != 0 and length > 0:
        padding = msb_mask
        for i in range(0, length-1):
            padding = padding >> 1
            padding = padding | msb_mask
        new_value = new_value | padding
    return new_value


def make_input_file(filename, inputs):
    with open(filename, 'w') as in_file:
        height = len(inputs[0])
        if(height < 1):
            return False
        in_file.write("a clk rst\n")
        for i in range(0, height):
            for j in range(0, len(inputs)):
                if j == 0:
                    in_file.write(hex(inputs[j][i]))
                else:
                    in_file.write(str(inputs[j][i]))
                in_file.write(" ")
            in_file.write("\n")
    return True


def make_input_vector(bits):
    value_index = 0
    clock_index = 1
    reset_index = 2
    value = 0
    inputs = []
    #Three columns
    inputs.append([])
    inputs.append([])
    inputs.append([])
    #One for each value (2^bits)
    #a rising edge for each line ( * 2)
    #a line for reset ( + 1)
    lines = ((2 ** bits) * 2) + 2
    for i in range(0, lines):
        if i < (lines - 1):
            inputs[0].append(value)
            inputs[1].append(i%2)
            inputs[2].append(0)
        else:
            value -= 1
            inputs[0].append(value)
            inputs[1].append(i%2)
            inputs[2].append(1)
        value += i%2
    return inputs


if __name__ == "__main__":
    if len(sys.argv) < 5:
        print("Usage: asr_vector_maker.py <input_vector_filename> <output_vector_filename> <number_of_bits> <shift_amount>")
    bits = int(sys.argv[3])
    length = int(sys.argv[4])
    inputs = make_input_vector(bits)
    outputs = make_output_vector(bits, length, inputs)
    if(make_input_file(sys.argv[1], inputs)):
        make_output_file(sys.argv[2], outputs)
    else:
        print("Failed to create input file")

