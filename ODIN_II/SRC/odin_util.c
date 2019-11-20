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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "types.h"
#include "globals.h"
#include "errors.h"
#include "odin_util.h"

/*--------------------------------------------------------------------------
 * (function: make_signal_name)
// return signal_name-bit
 *------------------------------------------------------------------------*/
char *make_signal_name(char *signal_name, int bit)
{
	char *return_string;

	oassert(signal_name != NULL);
	if (bit == -1)
		return strdup(signal_name);

	return_string = strdup(signal_name);
	return_string = (char*)realloc(return_string, sizeof(char)*(strlen(return_string)+1+10+1));
	sprintf(return_string, "%s-%d", return_string, bit);
	return return_string;	
}

/*---------------------------------------------------------------------------------------------
 * (function: make_full_ref_name)
// {previous_string}.module_name+instance_name
// {previous_string}.module_name+instance_name^signal_name
// {previous_string}.module_name+instance_name^signal_name~bit
 *-------------------------------------------------------------------------------------------*/
char *make_full_ref_name(char *previous, char *module_name, char *module_instance_name, char *signal_name, long bit)
{
	char *return_string = malloc(sizeof(char)*1);
	return_string[0] = '\0';

	if (previous)
	{
		free(return_string);
		return_string = strdup(previous);
	}
	if (module_name)
	{
		return_string = realloc(return_string,
				sizeof(char)*(
						 strlen(return_string)
						+1
						+strlen(module_name)
						+1
						+strlen(module_instance_name)
						+1
				)
		);
		sprintf(return_string, "%s.%s+%s", return_string, module_name, module_instance_name);
	}
	if (signal_name && (previous || module_name))
	{
		return_string = realloc(return_string, sizeof(char)*(strlen(return_string)+1+strlen(signal_name)+1));
		strcat(return_string, "^");
		strcat(return_string, signal_name);
	}
	else if (signal_name)
	{
		return_string = realloc(return_string, sizeof(char)*(strlen(return_string)+1+strlen(signal_name)+1));
		strcat(return_string, signal_name);
	}
	if (bit != -1)
	{
		oassert(signal_name != NULL);
		return_string = realloc(return_string, sizeof(char)*(strlen(return_string)+1+30+1));
		sprintf(return_string, "%s~%ld", return_string, bit);
	}

	return_string = realloc(return_string, sizeof(char)*strlen(return_string)+1);
	return return_string;	
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
		return convert_hex_string_of_size_to_bit_string(string, binary_size);
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
		return convert_binary_string_of_size_to_bit_string(string, binary_size);
	}
	else
	{
		return NULL;
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: convert_int_to_bit_string)
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
long long convert_dec_string_of_size_to_long_long(char *orig_string, int size)
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

	#ifdef LLONG_MAX
	long long number = strtoll(orig_string, NULL, radix);
	if (number == LLONG_MAX || number == LLONG_MIN)
		error_message(PARSE_ERROR, -1, -1, "This base %d number (%s) is too long for Odin\n", radix, orig_string);
	#else
	long long number = strtol(orig_string, NULL, radix);
	if (number == LONG_MAX || number == LONG_MIN)
		error_message(PARSE_ERROR, -1, -1, "This base %d number (%s) is too long for Odin\n", radix, orig_string);
	#endif

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
char *convert_hex_string_of_size_to_bit_string(char *orig_string, int binary_size)
{
	if (!is_hex_string(orig_string))
		error_message(PARSE_ERROR, -1, -1, "Invalid hex number: %s.\n", orig_string);

	char *bit_string = calloc(1,sizeof(char));
	char *string     = strdup(orig_string);
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
			bit_string = realloc(bit_string, sizeof(char) * (count + 2));
			bit_string[count++] = '0' + bit;
			bit_string[count]   = '\0';
		}
	}
	free(string);

	// Pad with zeros to binary_size.
	while (count < binary_size)
	{
		bit_string = realloc(bit_string, sizeof(char) * (count + 2));
		bit_string[count++] = '0';
		bit_string[count]   = '\0';
	}

	// Truncate to binary_size
	bit_string[binary_size] = '\0';
	// Change to little endian
	reverse_string(bit_string, binary_size);
	// Copy out only the bits before the truncation.
	char *return_string = strdup(bit_string);
	free(bit_string);
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

	char *bit_string = calloc(1,sizeof(char));
	char *string     = strdup(orig_string);
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
			bit_string = realloc(bit_string, sizeof(char) * (count + 2));
			bit_string[count++] = '0' + bit;
			bit_string[count]   = '\0';
		}
	}
	free(string);

	// Pad with zeros to binary_size.
	while (count < binary_size)
	{
		bit_string = realloc(bit_string, sizeof(char) * (count + 2));
		bit_string[count++] = '0';
		bit_string[count]   = '\0';
	}

	// Truncate to binary_size
	bit_string[binary_size] = '\0';
	// Change to little endian
	reverse_string(bit_string, binary_size);
	// Copy out only the bits before the truncation.
	char *return_string = strdup(bit_string);
	free(bit_string);
	return return_string;
}

/*
 * Parses the given little endian bit string into a bit string padded or truncated to
 * binary_size bits.
 */
char *convert_binary_string_of_size_to_bit_string(char *orig_string, int binary_size)
{
	if (!is_binary_string(orig_string))
		error_message(PARSE_ERROR, -1, -1, "Invalid binary number: %s.\n", orig_string);

	int   count      = strlen(orig_string);
	char *bit_string = calloc(count + 1, sizeof(char));

	// Copy the original string into the buffer.
	strcat(bit_string, orig_string);

	// Change to big endian.
	reverse_string(bit_string, count);

	// Pad with zeros to binary_size.
	while (count < binary_size)
	{
		bit_string = realloc(bit_string, sizeof(char) * (count + 2));
		bit_string[count++] = '0';
		bit_string[count]   = '\0';
	}

	// Truncate to binary_size
	bit_string[binary_size] = '\0';
	// Change to little endian
	reverse_string(bit_string, binary_size);
	// Copy out only the bits before the truncation.
	char *return_string = strdup(bit_string);
	free(bit_string);
	return return_string;
}

/*
 * Returns TRUE if the given string contains only '0' to '9' and 'a' through 'f'
 */
int is_hex_string(char *string)
{
	int i;
	for (i = 0; i < strlen(string); i++)
		if (!((string[i] >= '0' && string[i] <= '9') || (tolower(string[i]) >= 'a' && tolower(string[i]) <= 'f')))
			return FALSE;

	return TRUE;
}

/*
 * Returns TRUE if the string contains only '0' to '9'
 */
int is_decimal_string(char *string)
{
	int i;
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
	int i;
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
	int i;
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
		return strdup(strchr(name, '^') + 1);
	else
		return strdup(name);
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

	free(pin_name);
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
 *  (function: make_simple_name )
 *-------------------------------------------------------------------------------------------*/
char *make_string_based_on_id(nnode_t *node)
{
	char *return_string = (char*)malloc(sizeof(char)*(20+2)); // any unique id greater than 20 characters means trouble

	sprintf(return_string, "n%ld", node->unique_id);

	return return_string;
}

/*---------------------------------------------------------------------------------------------
 *  (function: make_simple_name )
 *-------------------------------------------------------------------------------------------*/
char *make_simple_name(char *input, char *flatten_string, char flatten_char)
{
	int i;
	int j;
	char *return_string = NULL;
	oassert(input != NULL);

	return_string = (char*)malloc(sizeof(char)*(strlen(input)+1));

	for (i = 0; i < strlen(input); i++)
	{ 
		return_string[i] = input[i];
		for (j = 0; j < strlen(flatten_string); j++)
		{
			if (input[i] == flatten_string[j])
			{
				return_string[i] = flatten_char;
				break;
			}
		}
	}

	return_string[strlen(input)] = '\0';	

	return return_string;
}

/*-----------------------------------------------------------------------
 * (function: my_malloc_struct )
 *-----------------------------------------------------------------*/
void *my_malloc_struct(size_t bytes_to_alloc)
{
	void *allocated = NULL;
	static long int m_id = 0;

	// ways to stop the execution at the point when a specific structure is built...note it needs to be m_id - 1 ... it's unique_id in most data structures
	//oassert(m_id != 193);

	allocated = malloc(bytes_to_alloc);
	if(allocated == NULL)
	{
		fprintf(stderr,"MEMORY FAILURE\n");
		oassert (0);
	}

	/* mark the unique_id */
	*((long int*)allocated) = m_id;

	m_id++;

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
		int i;
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
		int i;
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
char *append_string(char *string, char *appendage, ...)
{
	char buffer[BUFSIZE];

	va_list ap;

	va_start(ap, appendage);
	vsnprintf(buffer, BUFSIZE * sizeof(char), appendage, ap);
	va_end(ap);


	char *new_string = (char *)malloc(strlen(string) + strlen(buffer) + 1);
	strcpy(new_string, string);
	strcat(new_string, buffer);
	return new_string;
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
