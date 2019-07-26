
def make_vectors():
    input_vectors = []
    next_a = -1
    for i in range(0, 512):
        if i%2 == 0:
            next_a += 1
        input_vectors.append(str(hex(next_a)) + " " + str(i%2) + " " + str(0))
    input_vectors.append(str(hex(next_a)) + " " + str(0) + " " + str(1))
    return input_vectors

if __name__ == "__main__":
    inputs = make_vectors()
    for line in inputs:
        print(line)
