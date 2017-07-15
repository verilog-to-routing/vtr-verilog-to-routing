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

#include "odin_util.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include <regex>
#include <stdbool.h>

/*--------------------------------------------------------------------------
 * (function: make_signal_name)
// return signal_name-bit
 *------------------------------------------------------------------------*/
char *make_signal_name(char *signal_name, int bit)
{
	oassert(signal_name);
	std::stringstream return_string;
	return_string << signal_name;
	if (bit != -1) return_string << "-" << std::dec << bit;
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
	if(previous)								 return_string << previous;
	if(module_name) 							 return_string	<< "." << module_name << "+" << module_instance_name;
	if(signal_name && (previous || module_name)) return_string << "^";
	if(signal_name)								 return_string << signal_name;
	if(bit != -1){
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
char *convert_string_of_radix_to_bit_string(const char *string_in, int radix, int binary_size)
{
	if (!is_string_of_radix(string_in,radix))
		error_message(PARSE_ERROR, -1, -1, "Invalid base %d number: %s.\n", radix, string_in);
	
	if (is_dont_care_string(string_in))
		error_message(PARSE_ERROR, -1, -1, "Invalid number, contains x or z wich is unsupported: %s.\n", radix, string_in);
		
	std::string number(string_in);
	std::string output;
    switch(radix){
        case 2:     
        case 8:     
        case 16:
            return vtr::strdup(base_log2_convert(number, radix, binary_size, 0).c_str());
        case 10:
            return vtr::strdup(base_10_convert(number, binary_size).c_str());
        default:
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

long long convert_string_of_radix_to_long_long(char *orig_string, int radix)
{
	if (!is_string_of_radix(orig_string,radix))
		error_message(PARSE_ERROR, -1, -1, "Invalid base %d number: %s.\n", radix, orig_string);
	
	if (is_dont_care_string(orig_string))
		error_message(PARSE_ERROR, -1, -1, "Invalid number, contains x or z wich is unsupported: %s.\n", radix, orig_string);
		
	#ifdef LLONG_MAX
	long long number = strtoll(orig_string, NULL, radix);
	if (number == LLONG_MAX || number == LLONG_MIN)
		error_message(PARSE_ERROR, -1, -1, "This base %d number (%s) is too long for Odin\n", radix, orig_string);
	#else
	long number = strtol(orig_string, NULL, radix);
	if (number == LONG_MAX || number == LONG_MIN)
		error_message(PARSE_ERROR, -1, -1, "This base %d number (%s) is too long for Odin\n", radix, orig_string);
	#endif

	return number;
}


/*
 * Returns TRUE if the given string contains x or z
 */
int is_dont_care_string(const char *string_in)
{
	std::string bit_s(string_in);
	if(bit_s.find("x") != std::string::npos) return	TRUE ; 
	if(bit_s.find("z") != std::string::npos) return	TRUE ; 
	if(bit_s.find("X") != std::string::npos) return	TRUE ; 
	if(bit_s.find("Z") != std::string::npos) return	TRUE ;	
	return FALSE; 
}


int is_string_of_radix(const char *string, int radix)
{
	std::string temp(string);
	size_t i;
	switch(radix)
	{
		case 16:
			for (i = 0; i < temp.length(); i++)
				if (!((temp[i] >= '0' && temp[i] <= '9') || (tolower(temp[i]) >= 'a' && tolower(temp[i]) <= 'f')))
					return FALSE;
			return TRUE;
			
		case 10:
			for (i = 0; i < temp.length(); i++)
				if (!(temp[i] >= '0' && temp[i] <= '9'))
					return FALSE;
			return TRUE;
			
		case 8:
			for (i = 0; i < temp.length(); i++)
				if (!(temp[i] >= '0' && temp[i] <= '7'))
					return FALSE;
			return TRUE;
		case 2:
			for (i = 0; i < temp.length(); i++)
				if (!(temp[i] >= '0' && temp[i] <= '1'))
					return FALSE;
			return TRUE;
		default:
			return FALSE;
	}
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
	char *return_string = (char*)malloc(sizeof(char)*(20+2)); // any unique id greater than 20 characters means trouble

	sprintf(return_string, "n%ld", node->unique_id);

	return return_string;
}

/*---------------------------------------------------------------------------------------------
 *  (function: make_simple_name )
 *-------------------------------------------------------------------------------------------*/
char *make_simple_name(char *input, const char *flatten_string, char flatten_char)
{
	unsigned int i;
	unsigned int j;
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
		fprintf(stderr," (File: %s)", configuration.list_of_file_names[file]);
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
		fprintf(stderr," (File: %s)", configuration.list_of_file_names[file]);
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
			sprintf(line,"%s",tmp.c_str());
			break;
		case 2:
			tmp = vtr::replace_all(tmp,sKey,rKey);
			sprintf(line,"%s",tmp.c_str());
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
	sprintf(line,"%s",tmp.c_str());

	return line;
}

bool validate_string_regex(const char *str_in, const char *pattern_in)
{
    std::string str(str_in);
    std::regex pattern(pattern_in);
    
    if(std::regex_match (str.begin(), str.end(), pattern))
    	return true;
    	
	fprintf(stderr,"\nRETURNING FALSE\n");
	return false;
}



std::string base_log2_helper(const char in_digit, size_t radix){
    switch(radix){
        case 2:
            switch(tolower(in_digit)){
                case '0': return "0";
                case '1': return "1";
                case 'x': return "x";
                case 'z': return "z";
                default : break;
            }
            break;
        case 8:
            switch(tolower(in_digit)){
                case '0': return "000";
                case '1': return "001";
                case '2': return "010";
                case '3': return "011";
                case '4': return "100";
                case '5': return "101";
                case '6': return "110";
                case '7': return "111";
				case 'x': return "xxx";
				case 'z': return "zzz";
                default : break;
            }
            break;
            
        case 16:
            switch(tolower(in_digit)){
                case '0': return "0000";
                case '1': return "0001";
                case '2': return "0010";
                case '3': return "0011";
                case '4': return "0100";
                case '5': return "0101";
                case '6': return "0110";
                case '7': return "0111";
                case '8': return "1000";
                case '9': return "1001";
                case 'a': return "1010";
                case 'b': return "1011";
                case 'c': return "1100";
                case 'd': return "1101";
                case 'e': return "1110";
                case 'f': return "1111";
				case 'x': return "xxxx";
				case 'z': return "zzzz";
                default : break;
            }
            break;
        default: break;  
    }
    printf("_________not a valid bit______:   %c", in_digit);
    return nullptr;
}

std::string base_log2_convert(std::string number, size_t radix, size_t bit_length, int signed_numb){

	if(bit_length == 0){
		std::string resulting_binary("");
		
		for(size_t i = 0; i< number.length(); i++)
			resulting_binary += base_log2_helper(number[i],radix);
		
		return resulting_binary;	
	}else{
		char sign = '0'; 
		if(signed_numb)
			sign = base_log2_helper(number[0],radix)[0];
			
		std::string resulting_binary(bit_length, sign);
		int i = number.length()-1;
		int j = bit_length-1;
		
        while(i>=0 && j>= 0)
        {
            std::string converted_digit = base_log2_helper(number[i--], radix);
            
            int k =converted_digit.length()-1;
            while(k>=0 && j >= 0)
                resulting_binary[j--] = converted_digit[k--];
                
            if((k>=0 || i>=0) && j<0)
				warning_message(PARSE_ERROR, -1, -1, "number (%s) base (%d) will be truncated to (%d) bit like defined in verilog file\n", number.c_str(), radix, bit_length);
        }
        return resulting_binary;
    }
}

std::string base_10_convert(std::string number, size_t bit_length){
    if(bit_length ==0){
        std::string bit_string;
        
        while(number.length() > 0){
            
            int i=number.length()-1;
            //catch last remainder
            bit_string.insert(bit_string.begin(),((number[i]-'0')%2)+'0');
    
            for(;i>=0;i--){
                if(i>0)
                    number[i] = (number[i]-'0')/2 + ((number[i-1]-'0')%2)*5 +'0';
                else
                    number[i] = (number[i]-'0')/2 + '0';
            }
            
            //remove padded 0
            while(number.length() > 0 && number[0] == '0')
                number.erase(number.begin());
        }
        return bit_string;
    }else{
        std::string bit_string_sized(bit_length,'0');
        
        int loc = bit_string_sized.length()-1;
        while(loc >=0 && number.length() > 0){
            
            int i=number.length()-1;
            //catch last remainder
            bit_string_sized[loc--] = (number[i]-'0')%2 +'0';
    
            for(;i>=0;i--){
                //num = num/2+ msb%2
                if(i>0)
                    number[i] = (number[i]-'0')/2 + (number[i-1]-'0')%2*5 +'0';
                else
                    number[i] = (number[i]-'0')/2 + '0';
            }
            
            //remove padded 0
            while(number.length() > 0 && number[0] == '0')
                number.erase(number.begin());
        }
        if(number.length() > 0 && number[0] != '0')
        	warning_message(PARSE_ERROR, -1, -1, "number (%s) base (%d) will be truncated to (%d) bit like defined in verilog file\n", number.c_str(), 10, bit_length);
        	
        return bit_string_sized;
    }
}