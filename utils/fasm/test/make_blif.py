#!/usr/bin/env python3
"""
This is an utility script that generates a BLIF file with arbitrary number of
5 and 6 input LUTs. Along with LUTs their initialization content is placed
in a comment just before the '.names' directive.
"""
import random

def main():

    lut_count = 35

    print('.model top')
    print('.inputs  ' + ' '.join(['di{}'.format(i) for i in range(6)]))
    print('.outputs ' + ' '.join(['do{}'.format(i) for i in range(lut_count)]))

    for i in range(lut_count):
        n = random.choice((5, 6))
        ones = set([''.join([random.choice('01') for i in range(n)]) for j in range(2**n)])

        init = 0
        for one in ones:
            idx = 0
            for j, c in enumerate(one):
                if c == '1':
                    idx |= (1 << (len(one)-1-j))
            init |= 1 << idx

        init_str = ''.join(['1' if init & (1<<j) else '0' for j in reversed(range(2**n))])

        print('# {}\'b{}'.format(2**n, init_str))
        print('.names ' + ' '.join(['di{}'.format(j) for j in range(n)]) + ' do{} '.format(i))
        for one in ones:
            print('{} 1'.format(one))

if __name__ == "__main__":
    main()
