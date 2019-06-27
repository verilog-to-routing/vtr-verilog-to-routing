/* Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
 *           Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com) and
 *            Dr. Kenneth B. Kent (ken@unb.ca)
 *            for the Reconfigurable Computing Research Lab at the
 *             Univerity of New Brunswick in Fredericton, New Brunswick, Canada
 */

#include "internal_bits.hpp"

using namespace BitSpace;
int main(int argc, char **argv)
{
    size_t size=0;
    size = strtoul(argv[1],nullptr,10);
	VerilogBits my_bits(size,'x');
    printf("array_size(%zu) \n\n================\n", my_bits.size());

	std::cout << my_bits.to_string(false) << std::endl;
	
	for(size_t value = 0; value <8; value++)
	{
		for(size_t i=0; i<size; i++)
		{
			BitSpace::bit_value_t val = static_cast<BitSpace::bit_value_t>(value);
			printf("(%hhu)[%zu] : ",val,i);
			my_bits.set_bit(i,val);
			std::cout << my_bits.to_string(false) << std::endl;
		}
	}
}
