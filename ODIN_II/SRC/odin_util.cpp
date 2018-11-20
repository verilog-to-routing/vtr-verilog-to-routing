/*
Copyright (c) 2009 Peter Andrew Jamieson (jamieson.peter@gmail.com)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#include <string>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "types.h"
#include "globals.h"
#include <cstdarg>

#include "odin_util.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include <regex>
#include <stdbool.h>


/* string conversions */
const char *MULTI_PORT_MUX_string = "MULTI_PORT_MUX";
const char *FF_NODE_string = "FF_NODE";
const char *BUF_NODE_string = "BUF_NODE";
const char *INPUT_NODE_string = "INPUT_NODE";
const char *CLOCK_NODE_string = "CLOCK_NODE";
const char *OUTPUT_NODE_string = "OUTPUT_NODE";
const char *GND_NODE_string = "GND_NODE";
const char *VCC_NODE_string = "VCC_NODE";
const char *ADD_string = "ADD";
const char *MINUS_string = "MINUS";
const char *BITWISE_NOT_string = "BITWISE_NOT";
const char *BITWISE_AND_string = "BITWISE_AND";
const char *BITWISE_OR_string = "BITWISE_OR";
const char *BITWISE_NAND_string = "BITWISE_NAND";
const char *BITWISE_NOR_string = "BITWISE_NOR";
const char *BITWISE_XNOR_string = "BITWISE_XNOR";
const char *BITWISE_XOR_string = "BITWISE_XOR";
const char *LOGICAL_NOT_string = "LOGICAL_NOT";
const char *LOGICAL_OR_string = "LOGICAL_OR";
const char *LOGICAL_AND_string = "LOGICAL_AND";
const char *LOGICAL_NAND_string = "LOGICAL_NAND";
const char *LOGICAL_NOR_string = "LOGICAL_NOR";
const char *LOGICAL_XOR_string = "LOGICAL_XOR";
const char *LOGICAL_XNOR_string = "LOGICAL_XNOR";
const char *MULTIPLY_string = "MULTIPLY";
const char *DIVIDE_string = "DIVIDE";
const char *MODULO_string = "MODULO";
const char *LT_string = "LT";
const char *GT_string = "GT";
const char *LOGICAL_EQUAL_string = "LOGICAL_EQUAL";
const char *NOT_EQUAL_string = "NOT_EQUAL";
const char *LTE_string = "LTE";
const char *GTE_string = "GTE";
const char *SR_string = "SR";
const char *SL_string = "SL";
const char *CASE_EQUAL_string = "CASE_EQUAL";
const char *CASE_NOT_EQUAL_string = "CASE_NOT_EQUAL";
const char *ADDER_FUNC_string = "ADDER_FUNC";
const char *CARRY_FUNC_string = "CARRY_FUNC";
const char *MUX_2_string = "MUX_2";
const char *HARD_IP_string = "HARD_IP";
const char *MEMORY_string = "MEMORY";

/*---------------------------------------------------------------------------------------------
 * (function: node_name_based_on_op)
 * 	Get the string version of a node
 *-------------------------------------------------------------------------------------------*/
const char *node_name_based_on_op(nnode_t *node)
{
	const char *return_string;

	switch(node->type)
	{
		case MULTI_PORT_MUX:
			return_string = MULTI_PORT_MUX_string;
			break;
		case FF_NODE:
			return_string = FF_NODE_string;
			break;
		case BUF_NODE:
			return_string = BUF_NODE_string;
			break;
		case CLOCK_NODE:
			return_string = CLOCK_NODE_string;
			break;
		case INPUT_NODE:
			return_string = INPUT_NODE_string;
			break;
		case OUTPUT_NODE:
			return_string = OUTPUT_NODE_string;
			break;
		case GND_NODE:
			return_string = GND_NODE_string;
			break;
		case VCC_NODE:
			return_string = VCC_NODE_string;
			break;
		case ADD:
			return_string = ADD_string;
			break;
		case MINUS:
			return_string = MINUS_string;
			break;
		case BITWISE_NOT:
			return_string = BITWISE_NOT_string;
			break;
		case BITWISE_AND:
			return_string = BITWISE_AND_string;
			break;
		case BITWISE_OR:
			return_string = BITWISE_OR_string;
			break;
		case BITWISE_NAND:
			return_string = BITWISE_NAND_string;
			break;
		case BITWISE_NOR:
			return_string = BITWISE_NOR_string;
			break;
		case BITWISE_XNOR:
			return_string = BITWISE_XNOR_string;
			break;
		case BITWISE_XOR:
			return_string = BITWISE_XOR_string;
			break;
		case LOGICAL_NOT:
			return_string = LOGICAL_NOT_string;
			break;
		case LOGICAL_OR:
			return_string = LOGICAL_OR_string;
			break;
		case LOGICAL_AND:
			return_string = LOGICAL_AND_string;
			break;
		case LOGICAL_NOR:
			return_string = LOGICAL_NOR_string;
			break;
		case LOGICAL_NAND:
			return_string = LOGICAL_NAND_string;
			break;
		case LOGICAL_XOR:
			return_string = LOGICAL_XOR_string;
			break;
		case LOGICAL_XNOR:
			return_string = LOGICAL_XNOR_string;
			break;
		case MULTIPLY:
			return_string = MULTIPLY_string;
			break;
		case DIVIDE:
			return_string = DIVIDE_string;
			break;
		case MODULO:
			return_string = MODULO_string;
			break;
		case LT:
			return_string = LT_string;
			break;
		case GT:
			return_string = GT_string;
			break;
		case LOGICAL_EQUAL:
			return_string = LOGICAL_EQUAL_string;
			break;
		case NOT_EQUAL:
			return_string = NOT_EQUAL_string;
			break;
		case LTE:
			return_string = LTE_string;
			break;
		case GTE:
			return_string = GTE_string;
			break;
		case SR:
			return_string = SR_string;
			break;
		case SL:
			return_string = SL_string;
			break;
		case CASE_EQUAL:
			return_string = CASE_EQUAL_string;
			break;
		case CASE_NOT_EQUAL:
			return_string = CASE_NOT_EQUAL_string;
			break;
		case ADDER_FUNC:
			return_string = ADDER_FUNC_string;
			break;
		case CARRY_FUNC:
			return_string = CARRY_FUNC_string;
			break;
		case MUX_2:
			return_string = MUX_2_string;
			break;
		case MEMORY:
			return_string = MEMORY_string;
			break;
		case HARD_IP:
			return_string = HARD_IP_string;
			break;
		default:
			return_string = NULL;
			oassert(FALSE);
			break;
	}
	return return_string;
}

/*--------------------------------------------------------------------------
 * (function: make_signal_name)
// return signal_name-bit
 *------------------------------------------------------------------------*/
char *make_signal_name(char *signal_name, int bit)
{
	oassert(signal_name);
	std::stringstream return_string;
	return_string << signal_name;
	if (bit != -1) 
		return_string << "-" << std::dec << bit;
		
	return vtr::strdup(return_string.str().c_str());
}

/*---------------------------------------------------------------------------------------------
 * (function: make_full_ref_name)
// {previous_string}.module_name+instance_name
// {previous_string}.module_name+instance_name^signal_name
// {previous_string}.module_name+instance_name^signal_name~bit
 *-------------------------------------------------------------------------------------------*/
char *make_full_ref_name(const char *previous, char *module_name, char *module_instance_name, const char *signal_name, long bit)
{

	std::stringstream return_string;
	if(previous)								 
		return_string << previous;

	if(module_name) 							 
		return_string	<< "." << module_name << "+" << module_instance_name;

	if(signal_name && (previous || module_name)) 
		return_string << "^";

	if(signal_name)								 
		return_string << signal_name;

	if(bit != -1)
	{
		oassert(signal_name);
		return_string	<< "~" << std::dec << bit ;
	}
	return vtr::strdup(return_string.str().c_str());
}

/*---------------------------------------------------------------------------------------------
 * (function: twos_complement)
 * Changes a bit string to its twos complement value
 *-------------------------------------------------------------------------------------------*/
char *twos_complement(char *str)
{
	int length = strlen(str) - 1;
	int i;
	int flag = 0;

	for (i = length; i >= 0; i--)
	{
		if (flag)
			str[i] = (str[i] == '1') ? '0' : '1';

		if ((str[i] == '1') && (flag == 0))
			flag = 1;
	}
	return str;
}

/*
 * A wrapper for the other string to bit string conversion functions.
 * Converts an arbitrary length string of base 16, 10, 8, or 2 to a
 * string of 1s and 0s of the given length, padding or truncating
 * the higher accordingly. The returned string will be little
 * endian. Null will be returned if the radix is invalid.
 *
 * Base 10 strings will be limited in length to a long long, but
 * an error will be issued if the number will be truncated.
 *
 */
char *convert_string_of_radix_to_bit_string(char *string, int radix, int binary_size)
{
	if (radix == 16)
	{
		return convert_hex_string_of_size_to_bit_string(0, string, binary_size);
	}
	else if (radix == 10)
	{
		long long number = convert_dec_string_of_size_to_long_long(string, binary_size);
		return convert_long_long_to_bit_string(number, binary_size);
	}
	else if (radix == 8)
	{
		return convert_oct_string_of_size_to_bit_string(string, binary_size);
	}
	else if (radix == 2)
	{
		return convert_binary_string_of_size_to_bit_string(0, string, binary_size);
	}
	else
	{
		return NULL;
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: convert_long_long_to_bit_string)
 * Outputs a string msb to lsb.  For example, 3 becomes "011"
 *-------------------------------------------------------------------------------------------*/
char *convert_long_long_to_bit_string(long long orig_long, int num_bits)
{
	int i;
	char *return_val = (char*)malloc(sizeof(char)*(num_bits+1));
	int mask = 1;

	for (i = num_bits-1; i >= 0; i--)
	{
		if((mask & orig_long) > 0) { return_val[i] = '1'; }
		else                       { return_val[i] = '0'; }
		mask = mask << 1;
	}
	return_val[num_bits] = '\0';

	return return_val;
}

/*
 * Turns the given little endian decimal string into a long long. Throws an error if the
 * string contains non-digits or is larger or smaller than the allowable range of long long.
 */
long long convert_dec_string_of_size_to_long_long(char *orig_string, int /*size*/)
{
	if (!is_decimal_string(orig_string))
		error_message(PARSE_ERROR, -1, -1, "Invalid decimal number: %s.\n", orig_string);

	errno = 0;
	long long number = strtoll(orig_string, NULL, 10);
	if (errno == ERANGE)
		error_message(PARSE_ERROR, -1, -1, "This suspected decimal number (%s) is too long for Odin\n", orig_string);

	return number;
}

long long convert_string_of_radix_to_long_long(char *orig_string, int radix)
{
	if (!is_string_of_radix(orig_string, radix))
		error_message(PARSE_ERROR, -1, -1, "Invalid base %d number: %s.\n", radix, orig_string);

	long long number = strtoll(orig_string, NULL, radix);
	if (number == LLONG_MAX || number == LLONG_MIN)
		error_message(PARSE_ERROR, -1, -1, "This base %d number (%s) is too long for Odin\n", radix, orig_string);

	return number;
}

int is_string_of_radix(char *string, int radix)
{
	if (radix == 16)
		return is_hex_string(string);
	else if (radix == 10)
		return is_decimal_string(string);
	else if (radix == 8)
		return is_octal_string(string);
	else if (radix == 2)
		return is_binary_string(string);
	else
		return FALSE;
}

/*
 * Parses the given little endian hex string into a little endian bit string padded or truncated to
 * binary_size bits. Throws an error if there are non-hex characters in the input string.
 */
char *convert_hex_string_of_size_to_bit_string(short is_dont_care_number, char *orig_string, int binary_size)
{
    char *return_string = NULL;
    if(is_dont_care_number == 0){
	if (!is_hex_string(orig_string))
		error_message(PARSE_ERROR, -1, -1, "Invalid hex number: %s.\n", orig_string);

	char *bit_string = (char *)vtr::calloc(1,sizeof(char));
	char *string     = vtr::strdup(orig_string);
	int   size       = strlen(string);

	// Change to big endian. (We want to add higher order bits at the end.)
	reverse_string(string, size);

	int count = 0;
	int i;
	for (i = 0; i < size; i++)
	{
		char temp[] = {string[i],'\0'};

		unsigned long value = strtoul(temp, NULL, 16);
		int k;
		for (k = 0; k < 4; k++)
		{
			char bit = value % 2;
			value /= 2;
			bit_string = (char *)vtr::realloc(bit_string, sizeof(char) * (count + 2));
			bit_string[count++] = '0' + bit;
			bit_string[count]   = '\0';
		}
	}
	vtr::free(string);

	// Pad with zeros to binary_size.
	while (count < binary_size)
	{
		bit_string = (char *)vtr::realloc(bit_string, sizeof(char) * (count + 2));
		bit_string[count++] = '0';
		bit_string[count]   = '\0';
	}

	// Truncate to binary_size
	bit_string[binary_size] = '\0';
	// Change to little endian
	reverse_string(bit_string, binary_size);
	// Copy out only the bits before the truncation.
	return_string = vtr::strdup(bit_string);
	vtr::free(bit_string);

    }
    else if(is_dont_care_number == 1){
       char *string = vtr::strdup(orig_string);
       int   size       = strlen(string);
       char *bit_string = (char *)vtr::calloc(1,sizeof(char));
       int count = 0;
       int i;
       for (i = 0; i < size; i++)
	   {
		    //char temp[] = {string[i],'\0'};

		    //unsigned long value = strtoul(temp, NULL, 16);
		    int k;
		    for (k = 0; k < 4; k++)
		    {
			    //char bit = value % 2;
			    //value /= 2;
			    bit_string = (char *)vtr::realloc(bit_string, sizeof(char) * (count + 2));
			    bit_string[count++] = string[i];
			    bit_string[count]   = '\0';
		    }
	    }

        vtr::free(string);

        while (count < binary_size)
	    {
		    bit_string = (char *)vtr::realloc(bit_string, sizeof(char) * (count + 2));
		    bit_string[count++] = '0';
		    bit_string[count]   = '\0';
	    }

        bit_string[binary_size] = '\0';

        reverse_string(bit_string, binary_size);

        return_string = vtr::strdup(bit_string);
	    vtr::free(bit_string);



       // printf("bit_string %s",bit_string);
       // getchar();

        //printf("return_string %s", return_string);
        //getchar();
        //return return_string;

    }

    return return_string;
}

/*
 * Parses the given little endian octal string into a little endian bit string padded or truncated to
 * binary_size bits. Throws an error if the string contains non-octal digits.
 */
char *convert_oct_string_of_size_to_bit_string(char *orig_string, int binary_size)
{
	if (!is_octal_string(orig_string))
		error_message(PARSE_ERROR, -1, -1, "Invalid octal number: %s.\n", orig_string);

	char *bit_string = (char *)vtr::calloc(1,sizeof(char));
	char *string     = vtr::strdup(orig_string);
	int   size       = strlen(string);

	// Change to big endian. (We want to add higher order bits at the end.)
	reverse_string(string, size);

	int count = 0;
	int i;
	for (i = 0; i < size; i++)
	{
		char temp[] = {string[i],'\0'};

		unsigned long value = strtoul(temp, NULL, 8);
		int k;
		for (k = 0; k < 3; k++)
		{
			char bit = value % 2;
			value /= 2;
			bit_string = (char *)vtr::realloc(bit_string, sizeof(char) * (count + 2));
			bit_string[count++] = '0' + bit;
			bit_string[count]   = '\0';
		}
	}
	vtr::free(string);

	// Pad with zeros to binary_size.
	while (count < binary_size)
	{
		bit_string = (char *)vtr::realloc(bit_string, sizeof(char) * (count + 2));
		bit_string[count++] = '0';
		bit_string[count]   = '\0';
	}

	// Truncate to binary_size
	bit_string[binary_size] = '\0';
	// Change to little endian
	reverse_string(bit_string, binary_size);
	// Copy out only the bits before the truncation.
	char *return_string = vtr::strdup(bit_string);
	vtr::free(bit_string);
	return return_string;
}

/*
 * Parses the given little endian bit string into a bit string padded or truncated to
 * binary_size bits.
 */
char *convert_binary_string_of_size_to_bit_string(short is_dont_care_number, char *orig_string, int binary_size)
{
	if (!is_binary_string(orig_string) && !is_dont_care_number)
		error_message(PARSE_ERROR, -1, -1, "Invalid binary number: %s.\n", orig_string);

	int   count      = strlen(orig_string);
	char *bit_string = (char *)vtr::calloc(count + 1, sizeof(char));

	// Copy the original string into the buffer.
	strcat(bit_string, orig_string);

	// Change to big endian.
	reverse_string(bit_string, count);

	// Pad with zeros to binary_size.
	while (count < binary_size)
	{
		bit_string = (char *)vtr::realloc(bit_string, sizeof(char) * (count + 2));
		bit_string[count++] = '0';
		bit_string[count]   = '\0';
	}

	// Truncate to binary_size
	bit_string[binary_size] = '\0';
	// Change to little endian
	reverse_string(bit_string, binary_size);
	// Copy out only the bits before the truncation.
	char *return_string = vtr::strdup(bit_string);
	vtr::free(bit_string);
	return return_string;
}

/*
 * Returns TRUE if the given string contains only '0' to '9' and 'a' through 'f'
 */
int is_hex_string(char *string)
{
	unsigned int i;
	for (i = 0; i < strlen(string); i++)
		if (!((string[i] >= '0' && string[i] <= '9') || (tolower(string[i]) >= 'a' && tolower(string[i]) <= 'f')))
			return FALSE;

	return TRUE;
}

/*
 * Returns TRUE if the given string contains only '0' to '9' and 'a' through 'f'
 */
int is_dont_care_string(char *string)
{
	unsigned int i;
	for (i = 0; i < strlen(string); i++)
        if(string[i] != 'x') return FALSE;
		//if (!((string[i] >= '0' && string[i] <= '9') || (tolower(string[i]) >= 'a' && tolower(string[i]) <= 'f')))
		//	return FALSE;

	return TRUE;
}
/*
 * Returns TRUE if the string contains only '0' to '9'
 */
int is_decimal_string(char *string)
{
	unsigned int i;
	for (i = 0; i < strlen(string); i++)
		if (!(string[i] >= '0' && string[i] <= '9'))
			return FALSE;

	return TRUE;
}

/*
 * Returns TRUE if the string contains only '0' to '7'
 */
int is_octal_string(char *string)
{
	unsigned int i;
	for (i = 0; i < strlen(string); i++)
		if (!(string[i] >= '0' && string[i] <= '7'))
			return FALSE;

	return TRUE;
}

/*
 * Returns TRUE if the string contains only '0's and '1's.
 */
int is_binary_string(char *string)
{
	unsigned int i;
	for (i = 0; i < strlen(string); i++)
		if (!(string[i] >= '0' && string[i] <= '1'))
			return FALSE;

	return TRUE;
}

/*
 * Gets the port name (everything after the ^ character)
 * from the given name. Leaves the original name intact.
 * Returns a copy of the original name if there is no
 * ^ character present in the name.
 */
char *get_pin_name(char *name)
{	// Remove everything before the ^
	if (strchr(name, '^'))
		return vtr::strdup(strchr(name, '^') + 1);
	else
		return vtr::strdup(name);
}


/*
 * Gets the port name (everything after the ^ and before the ~)
 * from the given name.
 */
char *get_port_name(char *name)
{
	// Remove everything before the ^
	char *port_name = get_pin_name(name);
	// Find out if there is a ~ and remove everything after it.
	char *tilde = strchr(port_name, '~');
	if (tilde)
		*tilde = '\0';
	return port_name;
}

/*
 * Gets the pin number (the number after the ~)
 * from the given name.
 *
 * Returns -1 if there is no ~.
 */
int get_pin_number(char *name)
{
	// Grab the portion of the name ater the ^
	char *pin_name = get_pin_name(name);
	char *tilde = strchr(pin_name, '~');
	// The pin number is everything after the ~
	int pin_number;
	if (tilde) pin_number = strtol(tilde+1,NULL,10);
	else       pin_number = -1;

	vtr::free(pin_name);
	return pin_number;
}

/*---------------------------------------------------------------------------------------------
 * (function: my_power)
 *      My own simple power function
 *-------------------------------------------------------------------------------------------*/
long long int my_power(long long int x, long long int y)
{
	if (y == 0)
		return 1;

	long long int value = x;
	int i;
	for (i = 1; i < y; i++)
		value *= x;

	return value;
}

/*---------------------------------------------------------------------------------------------
 *  (function: make_string_based_on_id )
 *-------------------------------------------------------------------------------------------*/
char *make_string_based_on_id(nnode_t *node)
{
	// any unique id greater than 20 characters means trouble
	std::string return_string = std::string ("n") + std::to_string(node->unique_id);

	return vtr::strdup(return_string.c_str());
 }

/*---------------------------------------------------------------------------------------------
 *  (function: make_simple_name )
 *-------------------------------------------------------------------------------------------*/
std::string make_simple_name(char *input, const char *flatten_string, char flatten_char)
{
	oassert(input);
	oassert(flatten_string);

	std::string input_str = input;
	std::string flatten_str = flatten_string;

	for (int i = 0; i < flatten_str.length(); i++)
		std::replace( input_str.begin(), input_str.end(), flatten_str[i], flatten_char);

	return input_str;
}

/*-----------------------------------------------------------------------
 * (function: my_malloc_struct )
 *-----------------------------------------------------------------*/
void *my_malloc_struct(size_t bytes_to_alloc)
{
	void *allocated = vtr::calloc(1, bytes_to_alloc);
	static long int m_id = 0;

	// ways to stop the execution at the point when a specific structure is built...note it needs to be m_id - 1 ... it's unique_id in most data structures
	//oassert(m_id != 193);

	if(allocated == NULL)
	{
		fprintf(stderr,"MEMORY FAILURE\n");
		oassert (0);
	}

	/* mark the unique_id */
	*((long int*)allocated) = m_id++;

	return allocated;
}

/*---------------------------------------------------------------------------------------------
 * (function: pow2 )
 *-------------------------------------------------------------------------------------------*/
long long int pow2(int to_the_power)
{
	int i;
	long long int return_val = 1;

	for (i = 0; i < to_the_power; i++)
	{
		return_val = return_val << 1;
	}

	return return_val;
}

/*
 * Changes the given string to upper case.
 */
void string_to_upper(char *string)
{
	if (string)
	{
		unsigned int i;
		for (i = 0; i < strlen(string); i++)
		{
			string[i] = toupper(string[i]);
		}
	}
}

/*
 * Changes the given string to lower case.
 */
void string_to_lower(char *string)
{
	if (string)
	{
		unsigned int i;
		for (i = 0; i < strlen(string); i++)
		{
			string[i] = tolower(string[i]);
		}
	}
}

/*
 * Returns a new string consisting of the original string
 * plus the appendage. Leaves the original string
 * intact.
 *
 * Handles format strings as well.
 */
char *append_string(const char *string, const char *appendage, ...)
{
	char buffer[vtr::bufsize];

	va_list ap;

	va_start(ap, appendage);
	vsnprintf(buffer, vtr::bufsize * sizeof(char), appendage, ap);
	va_end(ap);

	std::string new_string = std::string(string) + std::string(buffer);
	return vtr::strdup(new_string.c_str());
}

/*
 * Reverses the given string. (Reverses only 'length'
 * chars from index 0 to length-1.)
 */
void reverse_string(char *string, int length)
{
	int i = 0;
	int j = length - 1;
	while(i < j)
	{
		char temp = string[i];
		string[i++] = string [j];
		string[j--] = temp;
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: to_bit)
 *-------------------------------------------------------------------------------------------*/
short get_bit(char in){
	if(in == 48 || in == 49)
		return (short)in-48;
	fprintf(stderr,"not a valid bit\n");
    return -1;
}


/*---------------------------------------------------------------------------------------------
 * (function: error_message)
 *-------------------------------------------------------------------------------------------*/
void error_message(short error_type, int line_number, int file, const char *message, ...)
{
	va_list ap;

	fprintf(stderr,"--------------\nOdin has decided you have failed ;)\n\n");

	fprintf(stderr,"ERROR:");
	if (file != -1)
		fprintf(stderr," (File: %s)", configuration.list_of_file_names[file].c_str());
	if (line_number > 0)
		fprintf(stderr," (Line number: %d)", line_number);
	if (message != NULL)
	{
		fprintf(stderr," ");
		va_start(ap, message);
		vfprintf(stderr,message, ap);
		va_end(ap);
	}

	if (message[strlen(message)-1] != '\n') fprintf(stderr,"\n");

	exit(error_type);
}

/*---------------------------------------------------------------------------------------------
 * (function: warning_message)
 *-------------------------------------------------------------------------------------------*/
void warning_message(short /*error_type*/, int line_number, int file, const char *message, ...)
{
	va_list ap;
	static short is_warned = FALSE;
	static long warning_count = 0;
	warning_count++;

	if (is_warned == FALSE) {
		fprintf(stderr,"-------------------------\nOdin has decided you may fail ... WARNINGS:\n\n");
		is_warned = TRUE;
	}

	fprintf(stderr,"WARNING (%ld):", warning_count);
	if (file != -1)
		fprintf(stderr," (File: %s)", configuration.list_of_file_names[file].c_str());
	if (line_number > 0)
		fprintf(stderr," (Line number: %d)", line_number);
	if (message != NULL) {
		fprintf(stderr," ");

		va_start(ap, message);
		vfprintf(stderr,message, ap);
		va_end(ap);
	}

	if (message[strlen(message)-1] != '\n') fprintf(stderr,"\n");
}

void passed_verify_i_o_availabilty(nnode_t *node, int expected_input_size, int expected_output_size, const char *current_src, int line_src) {
	if(!node)
		error_message(SIMULATION_ERROR, -1, -1, "node unavailable @%s::%s", current_src, line_src);

	std::stringstream err_message;
	int error=0;

	if(expected_input_size != -1 
	&& node->num_input_pins != expected_input_size) {
		err_message << " input size is " << std::to_string(node->num_input_pins) << " expected 3:\n";
		for(int i=0; i< node->num_input_pins; i++)
			err_message << "\t" << node->input_pins[0]->name << "\n";

		error=1;
	}

	if(expected_output_size != -1
	&& node->num_output_pins != expected_output_size) {
		err_message << " output size is " << std::to_string(node->num_output_pins) << " expected 1:\n";
		for(int i=0; i< node->num_output_pins; i++)
			err_message << "\t" << node->output_pins[0]->name << "\n";

		error=1;
	}

	if(error)
		error_message(SIMULATION_ERROR, -1, -1, "failed for %s:%s %s\n",node_name_based_on_op(node), node->name, err_message.str().c_str());
}


/*
Search and replace a string keeping original string intact
*/
char *search_replace(char *src, const char *sKey, const char *rKey, int flag)
{
	std::string tmp;
	char *line;
	line = vtr::strdup(src);
	tmp = line;
	switch(flag)
	{
		case 1:
			tmp = vtr::replace_first(tmp,sKey,rKey);
			odin_sprintf(line,"%s",tmp.c_str());
			break;
		case 2:
			tmp = vtr::replace_all(tmp,sKey,rKey);
			odin_sprintf(line,"%s",tmp.c_str());
			break;
		default:
			return line;
	}
	return line;
}
char *find_substring(char *src,const char *sKey,int flag)
{
	// flag == 1 first half, flag == 2 second half

	std::string tmp;
	std::string key;
	char *line;
	line = vtr::strdup(src);
	tmp = line;
	key = sKey;
	std::size_t found = tmp.find(key);
	switch(flag)
	{
		case 1:
   			tmp = tmp.substr(0,found-1);
			break;
		case 2:
   			tmp = tmp.substr(found,tmp.length());
			break;

		default:
			return line;
	}
	odin_sprintf(line,"%s",tmp.c_str());

	return line;
}

bool validate_string_regex(const char *str_in, const char *pattern_in)
{
	std::string str(str_in);
    std::regex pattern(pattern_in);

    auto words_begin = std::sregex_iterator(str.begin(), str.end(), pattern);
	auto words_end = std::sregex_iterator();
	if (std::distance(words_begin,words_end) > 0)
		return true;
	return false;
}

/*
 * Prints the time in appropriate units.
 */
void print_time(double time)
{
	if      (time > 24*3600) printf("%.1fd",  time/(24*3600.0));
	else if (time > 3600)    printf("%.1fh",  time/3600.0);
	else if (time > 60)      printf("%.1fm",  time/60.0);
	else if (time > 1)       printf("%.1fs",  time);
	else                     printf("%.1fms", time*1000);
}

/*
 * Gets the current time in seconds.
 */
double wall_time()
{
	typedef std::chrono::system_clock Time;
	typedef std::chrono::duration<double> dsec;
	auto time_point = Time::now();
	dsec time_since_epoch = time_point.time_since_epoch();

	return time_since_epoch.count();
}

std::string strip_path_and_ext(std::string file)
{
	std::string::size_type loc_path = file.find_last_of("/")+1;
	std::string::size_type loc_ext = file.find_last_of(".");
	return file.substr(loc_path, loc_ext-loc_path);
}

/*
 * Parses the given comma separated list
 */
std::vector<std::string> parse_seperated_list(char *list, const char *separator)
{
	std::vector<std::string> list_out;

	// Parse the list.
	if (!list)
		return list_out;


	char *pin_list = vtr::strdup(list);
	char *token    = strtok(pin_list, separator);
	while (token)
	{
		list_out.push_back(token);
		token = strtok(NULL, ",");
	}
	vtr::free(pin_list);
	return list_out;
}

/*
 * Prints/updates an ASCII progress bar of length "length" to position length * completion
 * from previous position "position". Updates ETA based on the elapsed time "time".
 * Returns the new position. If the position is unchanged the bar is not redrawn.
 *
 * Call with position = -1 to draw for the first time. Returns the new
 * position, calculated based on completion.
 */
int print_progress_bar(double completion, int position, int length, double time)
{
	if (position == -1 || ((int)(completion * length)) > position)
	{
		printf("%3.0f%%|", completion * (double)100);

		position = completion * length;

		int i;
		for (i = 0; i < position; i++)
			printf("=");

		printf(">");

		for (; i < length; i++)
			printf("-");

		if (completion < 1.0)
		{
			printf("| Remaining: ");
			double remaining_time = time/(double)completion - time;
			print_time(remaining_time);
		}
		else
		{
			printf("| Total time: ");
			print_time(time);
		}

		printf("    \r");

		if (position == length)
			printf("\n");

		fflush(stdout);
	}
	return position;
}

/*
 * Trims characters in the given "chars" string
 * from the end of the given string.
 */
void trim_string(char* string, const char *chars)
{
	if (string)
	{
		int length;
		while((length = strlen(string)))
		{	int trimmed = FALSE;
			unsigned int i;
			for (i = 0; i < strlen(chars); i++)
			{
				if (string[length-1] == chars[i])
				{
					trimmed = TRUE;
					string[length-1] = '\0';
					break;
				}
			}

			if (!trimmed)
				break;
		}
	}
}

/**
 * verifies only one condition evaluates to true
 */
bool only_one_is_true(std::vector<bool> tested)
{
	bool previous_value = false;
	for(bool next_value: tested)
	{
		if(!previous_value && next_value)
			previous_value = true;
		else if(previous_value && next_value)
			return false;
	}
	return previous_value;
}

/**
 * This overrides default sprintf since odin uses sprintf to concatenate strings
 * sprintf has undefined behavior for such and this prevents string overriding if 
 * it is also given as an input
 */
int odin_sprintf (char *s, const char *format, ...)
{
    va_list args, args_copy ;
    va_start( args, format ) ;
    va_copy( args_copy, args ) ;

    const auto sz = std::vsnprintf( nullptr, 0, format, args ) + 1 ;

    try
    {
        std::string temp( sz, ' ' ) ;
        std::vsnprintf( &temp.front(), sz, format, args_copy ) ;
        va_end(args_copy) ;
        va_end(args) ;

        s = strncpy(s, temp.c_str(),temp.length());

		return temp.length();

    }
    catch( const std::bad_alloc& )
    {
        va_end(args_copy) ;
        va_end(args) ;
        return -BUFFER_MAX_SIZE;
    }

}
 