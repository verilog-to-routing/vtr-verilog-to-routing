/*
 * Copyright (c) 2009 Peter Andrew Jamieson (jamieson.peter@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include <string>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "odin_types.h"
#include "odin_globals.h"
#include <cstdarg>

#include "odin_util.h"
#include "vtr_util.h"
#include "vtr_path.h"
#include "vtr_memory.h"
#include <regex>
#include <stdbool.h>

// for mkdir
#ifdef WIN32
#    include <direct.h>
#    define getcwd _getcwd
#else
#    include <sys/stat.h>
#    include <unistd.h>
#endif

long shift_left_value_with_overflow_check(long input_value, long shift_by, loc_t loc) {
    if (shift_by < 0)
        error_message(NETLIST, loc, "requesting a shift left that is negative [%ld]\n", shift_by);
    else if (shift_by >= (long)ODIN_STD_BITWIDTH - 1)
        warning_message(NETLIST, loc, "requesting a shift left that will overflow the maximum size of %ld [%ld]\n", shift_by, ODIN_STD_BITWIDTH - 1);

    return input_value << shift_by;
}

std::string get_file_extension(std::string input_file) {
    auto dot_location = input_file.find_last_of('.');
    if (dot_location != std::string::npos) {
        return input_file.substr(dot_location);
    } else {
        return "";
    }
}

std::string get_directory(std::string input_file) {
    auto last_slash_location = input_file.find_last_of("\\/");
    if (last_slash_location != std::string::npos) {
        return input_file.substr(0, last_slash_location);
    } else {
        return "";
    }
}

void create_directory(std::string path) {
    // CREATE OUTPUT DIRECTORY
    int error_code = 0;
#ifdef WIN32
    error_code = mkdir(path.c_str());
#else
    error_code = mkdir(path.c_str(), 0755);
#endif

    if (error_code && errno != EEXIST) {
        error_message(AST, unknown_location, "Odin Failed to create directory :%s with exit code%d\n", path.c_str(), errno);
    }
}

void assert_supported_file_extension(std::string input_file, loc_t loc) {
    bool supported = false;
    std::string extension = get_file_extension(input_file);
    for (int i = 0; i < file_type_e::file_type_e_END && !supported; i++) {
        supported = (file_type_strmap[extension] != file_type_e::file_type_e_END);
    }

    if (!supported) {
        std::string supported_extension_list = "";
        for (auto iter : file_type_strmap) {
            supported_extension_list += " ";
            supported_extension_list += iter.second;
        }

        possible_error_message(UTIL, loc,
                               "File (%s) has an unsupported extension (%s), Odin only supports { %s }",
                               input_file.c_str(),
                               extension.c_str(),
                               supported_extension_list.c_str());
    }
}

FILE* open_file(const char* file_name, const char* open_type) {
    FILE* opened_file = fopen(file_name, open_type);
    if (opened_file == NULL) {
        error_message(UTIL, unknown_location, "cannot open file: %s\n", file_name);
    }
    return opened_file;
}

/**
 * (function: get_current_path)
 *
 * @brief find the path where Odin-II is running
 */
void get_current_path() {
    /* create a string buffer to hold path */
    char* buffer;
    buffer = getcwd(NULL, READ_BUFFER_SIZE);

    global_args.current_path = std::string(buffer);
    vtr::free(buffer);
}

/*---------------------------------------------------------------------------------------------
 * (function: name_based_on_op)
 * 	Get the string version of an operation
 *-------------------------------------------------------------------------------------------*/
const char* name_based_on_op(operation_list op) {
    oassert(op < operation_list_END && "OUT OF BOUND operation_list!");

    return operation_list_STR[op][ODIN_STRING_TYPE];
}

/*---------------------------------------------------------------------------------------------
 * (function: name_based_on_ids)
 * 	Get the string version of an operation
 *-------------------------------------------------------------------------------------------*/
const char* name_based_on_ids(ids op) {
    oassert(op < ids_END && "OUT OF BOUND ids!");

    return ids_STR[op];
}

/*---------------------------------------------------------------------------------------------
 * (function: node_name_based_on_op)
 * 	Get the string version of a node
 *-------------------------------------------------------------------------------------------*/
const char* node_name_based_on_op(nnode_t* node) {
    return name_based_on_op(node->type);
}

/*---------------------------------------------------------------------------------------------
 * (function: node_name_based_on_ids)
 * 	Get the string version of a ast node
 *-------------------------------------------------------------------------------------------*/
const char* ast_node_name_based_on_ids(ast_node_t* node) {
    return name_based_on_ids(node->type);
}

/*--------------------------------------------------------------------------
 * (function: make_signal_name)
 * // return signal_name-bit
 *------------------------------------------------------------------------*/
char* make_signal_name(char* signal_name, int bit) {
    oassert(signal_name);
    std::stringstream return_string;
    return_string << signal_name;
    if (bit != -1)
        return_string << "-" << std::dec << bit;

    return vtr::strdup(return_string.str().c_str());
}

/*---------------------------------------------------------------------------------------------
 * (function: make_full_ref_name)
 * // {previous_string}.instance_name
 * // {previous_string}.instance_name^signal_name
 * // {previous_string}.instance_name^signal_name~bit
 *-------------------------------------------------------------------------------------------*/
char* make_full_ref_name(const char* previous, const char* /*module_name*/, const char* module_instance_name, const char* signal_name, long bit) {
    std::stringstream return_string;
    if (previous)
        return_string << previous;

    if (module_instance_name)
        return_string << "." << module_instance_name;

    if (signal_name && (previous || module_instance_name))
        return_string << "^";

    if (signal_name)
        return_string << signal_name;

    if (bit != -1) {
        oassert(signal_name);
        return_string << "~" << std::dec << bit;
    }
    return vtr::strdup(return_string.str().c_str());
}

/*---------------------------------------------------------------------------------------------
 * (function: make_full_name_w_o_array_ref)
 * // {previous_string}.module_name+instance_name
 *-------------------------------------------------------------------------------------------*/
char* make_full_name_w_o_array_ref(const char* previous, const char* /*module_name*/, const char* module_instance_name) {
    std::stringstream return_string;
    if (previous)
        return_string << previous;

    if (module_instance_name)
        return_string << "." << module_instance_name;

    std::string name = return_string.str();

    size_t idx = name.find_first_of('[', 0);
    if (idx != std::string::npos) {
        // delete array refs
        name.erase(idx, std::string::npos);
    }

    return vtr::strdup(name.c_str());
}

/*---------------------------------------------------------------------------------------------
 * (function: twos_complement)
 * Changes a bit string to its twos complement value
 *-------------------------------------------------------------------------------------------*/
char* twos_complement(char* str) {
    int length = strlen(str) - 1;
    int i;
    int flag = 0;

    for (i = length; i >= 0; i--) {
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
 * Base 10 strings will be limited in length to a long, but
 * an error will be issued if the number will be truncated.
 *
 */
char* convert_string_of_radix_to_bit_string(char* string, int radix, int binary_size) {
    if (radix == 16) {
        return convert_hex_string_of_size_to_bit_string(0, string, binary_size);
    } else if (radix == 10) {
        long number = convert_dec_string_of_size_to_long(string, binary_size);
        return convert_long_to_bit_string(number, binary_size);
    } else if (radix == 8) {
        return convert_oct_string_of_size_to_bit_string(string, binary_size);
    } else if (radix == 2) {
        return convert_binary_string_of_size_to_bit_string(0, string, binary_size);
    } else {
        return NULL;
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: convert_long_to_bit_string)
 * Outputs a string msb to lsb.  For example, 3 becomes "011"
 *-------------------------------------------------------------------------------------------*/
char* convert_long_to_bit_string(long orig_long, int num_bits) {
    int i;
    char* return_val = (char*)malloc(sizeof(char) * (num_bits + 1));
    int mask = 1;

    for (i = num_bits - 1; i >= 0; i--) {
        if ((mask & orig_long) > 0) {
            return_val[i] = '1';
        } else {
            return_val[i] = '0';
        }
        mask = mask << 1;
    }
    return_val[num_bits] = '\0';

    return return_val;
}

/*
 * Turns the given little endian decimal string into a long. Throws an error if the
 * string contains non-digits or is larger or smaller than the allowable range of long.
 */
long convert_dec_string_of_size_to_long(char* orig_string, int /*size*/) {
    if (!is_decimal_string(orig_string))
        error_message(UTIL, unknown_location, "Invalid decimal number: %s.\n", orig_string);

    errno = 0;
    long number = strtoll(orig_string, NULL, 10);
    if (errno == ERANGE)
        error_message(UTIL, unknown_location, "This suspected decimal number (%s) is too long for Odin\n", orig_string);

    return number;
}

long convert_string_of_radix_to_long(char* orig_string, int radix) {
    if (!is_string_of_radix(orig_string, radix))
        error_message(UTIL, unknown_location, "Invalid base %d number: %s.\n", radix, orig_string);

    long number = strtoll(orig_string, NULL, radix);
    if (number == LLONG_MAX || number == LLONG_MIN)
        error_message(UTIL, unknown_location, "This base %d number (%s) is too long for Odin\n", radix, orig_string);

    return number;
}

int is_string_of_radix(char* string, int radix) {
    if (radix == 16)
        return is_hex_string(string);
    else if (radix == 10)
        return is_decimal_string(string);
    else if (radix == 8)
        return is_octal_string(string);
    else if (radix == 2)
        return is_binary_string(string);
    else
        return false;
}

/*
 * Parses the given little endian hex string into a little endian bit string padded or truncated to
 * binary_size bits. Throws an error if there are non-hex characters in the input string.
 */
char* convert_hex_string_of_size_to_bit_string(short is_dont_care_number, char* orig_string, int binary_size) {
    char* return_string = NULL;
    if (is_dont_care_number == 0) {
        if (!is_hex_string(orig_string))
            error_message(UTIL, unknown_location, "Invalid hex number: %s.\n", orig_string);

        char* bit_string = (char*)vtr::calloc(1, sizeof(char));
        char* string = vtr::strdup(orig_string);
        int size = strlen(string);

        // Change to big endian. (We want to add higher order bits at the end.)
        reverse_string(string, size);

        int count = 0;
        int i;
        for (i = 0; i < size; i++) {
            char temp[] = {string[i], '\0'};

            unsigned long value = strtoul(temp, NULL, 16);
            int k;
            for (k = 0; k < 4; k++) {
                char bit = value % 2;
                value /= 2;
                bit_string = (char*)vtr::realloc(bit_string, sizeof(char) * (count + 2));
                bit_string[count++] = '0' + bit;
                bit_string[count] = '\0';
            }
        }
        vtr::free(string);

        // Pad with zeros to binary_size.
        while (count < binary_size) {
            bit_string = (char*)vtr::realloc(bit_string, sizeof(char) * (count + 2));
            bit_string[count++] = '0';
            bit_string[count] = '\0';
        }

        // Truncate to binary_size
        bit_string[binary_size] = '\0';
        // Change to little endian
        reverse_string(bit_string, binary_size);
        // Copy out only the bits before the truncation.
        return_string = vtr::strdup(bit_string);
        vtr::free(bit_string);

    } else if (is_dont_care_number == 1) {
        char* string = vtr::strdup(orig_string);
        int size = strlen(string);
        char* bit_string = (char*)vtr::calloc(1, sizeof(char));
        int count = 0;
        int i;
        for (i = 0; i < size; i++) {
            //char temp[] = {string[i],'\0'};

            //unsigned long value = strtoul(temp, NULL, 16);
            int k;
            for (k = 0; k < 4; k++) {
                //char bit = value % 2;
                //value /= 2;
                bit_string = (char*)vtr::realloc(bit_string, sizeof(char) * (count + 2));
                bit_string[count++] = string[i];
                bit_string[count] = '\0';
            }
        }

        vtr::free(string);

        while (count < binary_size) {
            bit_string = (char*)vtr::realloc(bit_string, sizeof(char) * (count + 2));
            bit_string[count++] = '0';
            bit_string[count] = '\0';
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
char* convert_oct_string_of_size_to_bit_string(char* orig_string, int binary_size) {
    if (!is_octal_string(orig_string))
        error_message(UTIL, unknown_location, "Invalid octal number: %s.\n", orig_string);

    char* bit_string = (char*)vtr::calloc(1, sizeof(char));
    char* string = vtr::strdup(orig_string);
    int size = strlen(string);

    // Change to big endian. (We want to add higher order bits at the end.)
    reverse_string(string, size);

    int count = 0;
    int i;
    for (i = 0; i < size; i++) {
        char temp[] = {string[i], '\0'};

        unsigned long value = strtoul(temp, NULL, 8);
        int k;
        for (k = 0; k < 3; k++) {
            char bit = value % 2;
            value /= 2;
            bit_string = (char*)vtr::realloc(bit_string, sizeof(char) * (count + 2));
            bit_string[count++] = '0' + bit;
            bit_string[count] = '\0';
        }
    }
    vtr::free(string);

    // Pad with zeros to binary_size.
    while (count < binary_size) {
        bit_string = (char*)vtr::realloc(bit_string, sizeof(char) * (count + 2));
        bit_string[count++] = '0';
        bit_string[count] = '\0';
    }

    // Truncate to binary_size
    bit_string[binary_size] = '\0';
    // Change to little endian
    reverse_string(bit_string, binary_size);
    // Copy out only the bits before the truncation.
    char* return_string = vtr::strdup(bit_string);
    vtr::free(bit_string);
    return return_string;
}

/*
 * Parses the given little endian bit string into a bit string padded or truncated to
 * binary_size bits.
 */
char* convert_binary_string_of_size_to_bit_string(short is_dont_care_number, char* orig_string, int binary_size) {
    if (!is_binary_string(orig_string) && !is_dont_care_number)
        error_message(UTIL, unknown_location, "Invalid binary number: %s.\n", orig_string);

    int count = strlen(orig_string);
    char* bit_string = (char*)vtr::calloc(count + 1, sizeof(char));

    // Copy the original string into the buffer.
    strcat(bit_string, orig_string);

    // Change to big endian.
    reverse_string(bit_string, count);

    // Pad with zeros to binary_size.
    while (count < binary_size) {
        bit_string = (char*)vtr::realloc(bit_string, sizeof(char) * (count + 2));
        bit_string[count++] = '0';
        bit_string[count] = '\0';
    }

    // Truncate to binary_size
    bit_string[binary_size] = '\0';
    // Change to little endian
    reverse_string(bit_string, binary_size);
    // Copy out only the bits before the truncation.
    char* return_string = vtr::strdup(bit_string);
    vtr::free(bit_string);
    return return_string;
}

/*
 * Returns true if the given string contains only '0' to '9' and 'a' through 'f'
 */
int is_hex_string(char* string) {
    unsigned int i;
    for (i = 0; i < strlen(string); i++)
        if (!((string[i] >= '0' && string[i] <= '9') || (tolower(string[i]) >= 'a' && tolower(string[i]) <= 'f')))
            return false;

    return true;
}

/*
 * Returns true if the given string contains only '0' to '9' and 'a' through 'f'
 */
int is_dont_care_string(char* string) {
    unsigned int i;
    for (i = 0; i < strlen(string); i++)
        if (string[i] != 'x') return false;
    //if (!((string[i] >= '0' && string[i] <= '9') || (tolower(string[i]) >= 'a' && tolower(string[i]) <= 'f')))
    //	return false;

    return true;
}
/*
 * Returns true if the string contains only '0' to '9'
 */
int is_decimal_string(char* string) {
    unsigned int i;
    for (i = 0; i < strlen(string); i++)
        if (!(string[i] >= '0' && string[i] <= '9'))
            return false;

    return true;
}

/*
 * Returns true if the string contains only '0' to '7'
 */
int is_octal_string(char* string) {
    unsigned int i;
    for (i = 0; i < strlen(string); i++)
        if (!(string[i] >= '0' && string[i] <= '7'))
            return false;

    return true;
}

/*
 * Returns true if the string contains only '0's and '1's.
 */
int is_binary_string(char* string) {
    unsigned int i;
    for (i = 0; i < strlen(string); i++)
        if (!(string[i] >= '0' && string[i] <= '1'))
            return false;

    return true;
}

/*
 * Gets the port name (everything after the ^ character)
 * from the given name. Leaves the original name intact.
 * Returns a copy of the original name if there is no
 * ^ character present in the name.
 */
char* get_pin_name(char* name) { // Remove everything before the ^
    if (strchr(name, '^'))
        return vtr::strdup(strchr(name, '^') + 1);
    else
        return vtr::strdup(name);
}

/*
 * Gets the port name (everything after the ^ and before the ~)
 * from the given name.
 */
char* get_port_name(char* name) {
    // Remove everything before the ^
    char* port_name = get_pin_name(name);
    // Find out if there is a ~ and remove everything after it.
    char* tilde = strchr(port_name, '~');
    if (tilde)
        *tilde = '\0';
    return port_name;
}

/**
 * (function: get_node_name)
 * 
 * @brief Removing the hard block unique number from its name 
 * and gets the node name (everything before the ~).
 *  
 * @param name the given hard block name
 * 
 * @return pure hard block name
 */
char* get_hard_block_node_name(char* name) {
    char* port_name = vtr::strdup(name);
    // Find out if there is a ~ and remove everything after it.
    char* tilde = strchr(port_name, '~');
    if (tilde)
        *tilde = '\0';
    return (port_name);
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: get_stripped_name)
 * 
 * @brief find the sub-circuit name in an altered sub-circuit name
 * In yosys cases, it appears when there is an instantiated parameterized 
 * module, so Yosys changes the name to avoid name collision. For Odin-II,
 * it looks for the pattern specified as names of supported hard blocks, 
 * such as mult_XXX
 * 
 * @param subcircuit_name complete name
 * 
 * @return a stripped name
 * -------------------------------------------------------------------------------------------
 */
char* get_stripped_name(const char* subcircuit_name) {
    /* validation */
    oassert(subcircuit_name);

    char* subcircuit_stripped_name = NULL;

    /* looking for Yosys style generated RTLIL module name */
    if (configuration.coarsen) {
        const char* pos = strchr(subcircuit_name, '\\');
        if (pos) {
            const char* end = strchr(pos, '\0');
            // get stripped name
            if (end) {
                subcircuit_stripped_name = (char*)vtr::malloc((end - pos + 1) * sizeof(char));
                memcpy(subcircuit_stripped_name, pos + 1, end - pos - 1);
                subcircuit_stripped_name[end - pos - 1] = '\0';
            }
        }
    }
    /* looking for Odin-II style subckt types */
    else {
        /* init sub-circuit */
        subcircuit_stripped_name = (char*)vtr::calloc(6, sizeof(char));
        /* Determine the type of hard block. */
        memcpy(subcircuit_stripped_name, subcircuit_name, 5);
        subcircuit_stripped_name[5] = '\0';
    }

    if (subcircuit_stripped_name == NULL)
        return (vtr::strdup(subcircuit_name));

    return (subcircuit_stripped_name);
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: retrieve_node_type_from_subckt_name)
 * 
 * @brief to retrieve the actual node type from the subcircuit name 
 * in cases where yosys generates a weird name, which includes port 
 * widths and additional information in a subcircuit name 
 * 
 * @param stripped_name subcircuit irregular name
 * 
 * @return the actual subcircuit name if it was successfully 
 * retrieved, otherwise NULL pointer
 * -------------------------------------------------------------------------------------------
 */
char* retrieve_node_type_from_subckt_name(const char* subcircuit_name) {
    /* validation */
    oassert(subcircuit_name);

    /* looking for Yosys style generated RTLIL module name */
    if (true) {
        for (auto pair : yosys_subckt_strmap)
            if (std::string(subcircuit_name).find(pair.first, 0) != std::string::npos)
                return vtr::strdup(pair.first.c_str());
    }

    return (NULL);
}

/*
 * Gets the pin number (the number after the ~)
 * from the given name.
 *
 * Returns -1 if there is no ~.
 */
int get_pin_number(char* name) {
    // Grab the portion of the name ater the ^
    char* pin_name = get_pin_name(name);
    char* tilde = strchr(pin_name, '~');
    // The pin number is everything after the ~
    int pin_number;
    if (tilde)
        pin_number = strtol(tilde + 1, NULL, 10);
    else
        pin_number = -1;

    vtr::free(pin_name);
    return pin_number;
}

/*---------------------------------------------------------------------------------------------
 * (function: my_power)
 *      My own simple power function
 *-------------------------------------------------------------------------------------------*/
long int my_power(long int x, long int y) {
    if (y == 0)
        return 1;

    long int value = x;
    int i;
    for (i = 1; i < y; i++)
        value *= x;

    return value;
}

/*---------------------------------------------------------------------------------------------
 *  (function: make_string_based_on_id )
 *-------------------------------------------------------------------------------------------*/
char* make_string_based_on_id(nnode_t* node) {
    // any unique id greater than 20 characters means trouble
    std::string return_string = std::string("n") + std::to_string(node->unique_id);

    return vtr::strdup(return_string.c_str());
}

/*---------------------------------------------------------------------------------------------
 *  (function: make_simple_name )
 *-------------------------------------------------------------------------------------------*/
std::string make_simple_name(char* input, const char* flatten_string, char flatten_char) {
    oassert(input);
    oassert(flatten_string);

    std::string input_str = input;
    std::string flatten_str = flatten_string;

    for (size_t i = 0; i < flatten_str.length(); i++)
        std::replace(input_str.begin(), input_str.end(), flatten_str[i], flatten_char);

    return input_str;
}

/*-----------------------------------------------------------------------
 * (function: my_malloc_struct )
 *-----------------------------------------------------------------*/
void* my_malloc_struct(long bytes_to_alloc) {
    void* allocated = vtr::calloc(1, bytes_to_alloc);
    static long int m_id = 0;

    // ways to stop the execution at the point when a specific structure is built...note it needs to be m_id - 1 ... it's unique_id in most data structures
    //oassert(m_id != 193);

    if (allocated == NULL) {
        fprintf(stderr, "MEMORY FAILURE\n");
        oassert(0);
    }

    /* mark the unique_id */
    *((long int*)allocated) = m_id++;

    return allocated;
}

/*---------------------------------------------------------------------------------------------
 * (function: pow2 )
 *-------------------------------------------------------------------------------------------*/
long int pow2(int to_the_power) {
    int i;
    long int return_val = 1;

    for (i = 0; i < to_the_power; i++) {
        return_val = return_val << 1;
    }

    return return_val;
}

/*
 * Changes the given string to upper case.
 */
void string_to_upper(char* string) {
    if (string) {
        unsigned int i;
        for (i = 0; i < strlen(string); i++) {
            string[i] = toupper(string[i]);
        }
    }
}

/*
 * Changes the given string to lower case.
 */
void string_to_lower(char* string) {
    if (string) {
        unsigned int i;
        for (i = 0; i < strlen(string); i++) {
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
char* append_string(const char* string, const char* appendage, ...) {
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
void reverse_string(char* string, int length) {
    int i = 0;
    int j = length - 1;
    while (i < j) {
        char temp = string[i];
        string[i++] = string[j];
        string[j--] = temp;
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: to_bit)
 *-------------------------------------------------------------------------------------------*/
short get_bit(char in) {
    if (in == 48 || in == 49)
        return (short)in - 48;
    fprintf(stderr, "not a valid bit\n");
    return -1;
}

/*---------------------------------------------------------------------------------------------
 * (function: to_bit)
 *-------------------------------------------------------------------------------------------*/
short get_bit(short in) {
    if (in == 0 || in == 1)
        return in;
    fprintf(stderr, "not a valid bit\n");
    return -1;
}

void passed_verify_i_o_availabilty(nnode_t* node, int expected_input_size, int expected_output_size, const char* current_src, int line_src) {
    if (!node)
        error_message(UTIL, unknown_location, "node unavailable @%s::%d", current_src, line_src);

    std::stringstream err_message;
    int error = 0;

    if (expected_input_size != -1
        && node->num_input_pins != expected_input_size) {
        err_message << " input size is " << std::to_string(node->num_input_pins) << " expected 3:\n";
        for (int i = 0; i < node->num_input_pins; i++)
            err_message << "\t" << node->input_pins[0]->name << "\n";

        error = 1;
    }

    if (expected_output_size != -1
        && node->num_output_pins != expected_output_size) {
        err_message << " output size is " << std::to_string(node->num_output_pins) << " expected 1:\n";
        for (int i = 0; i < node->num_output_pins; i++)
            err_message << "\t" << node->output_pins[0]->name << "\n";

        error = 1;
    }

    if (error)
        error_message(UTIL, node->loc, "failed for %s:%s %s\n", node_name_based_on_op(node), node->name, err_message.str().c_str());
}

/*
 * Search and replace a string keeping original string intact
 */
char* search_replace(char* src, const char* sKey, const char* rKey, int flag) {
    std::string tmp;
    char* line;
    line = vtr::strdup(src);
    tmp = line;
    switch (flag) {
        case 1:
            tmp = vtr::replace_first(tmp, sKey, rKey);
            odin_sprintf(line, "%s", tmp.c_str());
            break;
        case 2:
            tmp = vtr::replace_all(tmp, sKey, rKey);
            odin_sprintf(line, "%s", tmp.c_str());
            break;
        default:
            return line;
    }
    return line;
}
std::string find_substring(char* src, const char* sKey, int flag) {
    // flag == 1 first half, flag == 2 second half

    std::string tmp(src);
    std::string key(sKey);
    long found = tmp.find(key);
    switch (flag) {
        case 1:
            return tmp.substr(0, found - 1);

        case 2:
            return tmp.substr(found, tmp.length());

        default:
            return tmp;
    }
}

/*
 * Prints the time in appropriate units.
 */
void print_time(double time) {
    printf("%.1fms", time * 1000);
}

/*
 * Gets the current time in seconds.
 */
double wall_time() {
    auto time_point = std::chrono::system_clock::now();
    std::chrono::duration<double> time_since_epoch = time_point.time_since_epoch();

    return time_since_epoch.count();
}

/*
 * Prints/updates an ASCII progress bar of length "length" to position length * completion
 * from previous position "position". Updates ETA based on the elapsed time "time".
 * Returns the new position. If the position is unchanged the bar is not redrawn.
 *
 * Call with position = -1 to draw for the first time. Returns the new
 * position, calculated based on completion.
 */
int print_progress_bar(double completion, int position, int length, double time) {
    if (position == -1 || ((int)(completion * length)) > position) {
        printf("%3.0f%%|", completion * (double)100);

        position = completion * length;

        int i;
        for (i = 0; i < position; i++)
            printf("=");

        printf(">");

        for (; i < length; i++)
            printf("-");

        if (completion < 1.0) {
            printf("| Remaining: ");
            double remaining_time = time / (double)completion - time;
            print_time(remaining_time);
        } else {
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
void trim_string(char* string, const char* chars) {
    if (string) {
        int length;
        while ((length = strlen(string))) {
            bool trimmed = false;
            unsigned int i;
            for (i = 0; i < strlen(chars); i++) {
                if (string[length - 1] == chars[i]) {
                    trimmed = true;
                    string[length - 1] = '\0';
                    break;
                }
            }

            if (!trimmed)
                break;
        }
    }
}

/*
 * Tokenizes a string and returns an array where each element is a 
 * token and a pointer (that needs to be passed through) with the 
 * size of the array
 */
static char** string_to_token_array(char* string, int* size) {
    char** arr = NULL;

    char* token = strtok(string, " \n\t");
    int i = 0;

    while (token != NULL) {
        arr = (char**)vtr::realloc(arr, sizeof(char*) * (i + 1));
        arr[i] = token;
        token = strtok(NULL, " \n\t");
        i++;
    }
    *size = (i);
    return arr;
}

/*
 * Will seperate the headers from the inputs and compare them
 * Will return value of 1 if they match and 0 if
 * they don't match. Possible error if the vector headers match
 * but are in different order
 */
bool output_vector_headers_equal(char* buffer1, char* buffer2) {
    int size1 = 0;
    int size2 = 0;
    char* buffer1_copy = strdup(buffer1);
    char* buffer2_copy = strdup(buffer2);

    char** header1 = string_to_token_array(buffer1_copy, &size1);
    char** header2 = string_to_token_array(buffer2_copy, &size2);

    int i = 0;
    bool header_matches;

    header_matches = (size1 == size2);
    for (i = 0; header_matches && i < (size1 - 1); i++) {
        header_matches = (0 == strcmp(header1[i], header2[i]));
    }

    vtr::free(header1);
    vtr::free(header2);
    vtr::free(buffer1_copy);
    vtr::free(buffer2_copy);
    return (header_matches);
}

/**
 * verifies only one condition evaluates to true
 */
bool only_one_is_true(std::vector<bool> tested) {
    bool previous_value = false;
    for (bool next_value : tested) {
        if (!previous_value && next_value)
            previous_value = true;
        else if (previous_value && next_value)
            return false;
    }
    return previous_value;
}

/**
 * This overrides default sprintf since odin uses sprintf to concatenate strings
 * sprintf has undefined behavior for such and this prevents string overriding if 
 * it is also given as an input
 */
int odin_sprintf(char* s, const char* format, ...) {
    va_list args, args_copy;
    va_start(args, format);
    va_copy(args_copy, args);

    const auto sz = std::vsnprintf(nullptr, 0, format, args) + 1;

    try {
        std::string temp(sz, ' ');
        std::vsnprintf(&temp.front(), sz, format, args_copy);
        va_end(args_copy);
        va_end(args);

        s = strncpy(s, temp.c_str(), temp.length());

        return temp.length();

    } catch (const std::bad_alloc&) {
        va_end(args_copy);
        va_end(args);
        return -1;
    }
}

/**
 * This collate two strings together and destroys the original ones
 */
char* str_collate(char* str1, char* str2) {
    char* buffer = NULL;
    if (str1 && !str2) {
        buffer = str1;
    } else if (!str1 && str2) {
        buffer = str2;
    } else if (str1 && str2) {
        std::string _s1(str1);
        std::string _s2(str2);

        size_t pos = _s1.find_last_of('"');
        if (pos != std::string::npos) {
            _s1 = _s1.substr(0, pos);
        }

        pos = _s2.find_first_of('"');
        if (pos != std::string::npos) {
            _s2 = _s2.substr(pos + 1);
        }

        std::string result = _s1 + _s2;
        buffer = vtr::strdup(result.c_str());
        vtr::free(str1);
        vtr::free(str2);
    }
    return buffer;
}

/**
 * (function: print_input_files_info)
 * 
 * @brief This shows the name of input file, whether Verilog or BLIF
 */
void print_input_files_info() {
    if (configuration.input_file_type == file_type_e::_VERILOG) {
        for (std::string v_file : global_args.verilog_files.value())
            printf("Verilog: %s\n", vtr::basename(v_file).c_str());

    } else if (configuration.input_file_type == file_type_e::_BLIF) {
        printf("Input BLIF file: %s\n", vtr::basename(global_args.blif_file.value()).c_str());
    }

    fflush(stdout);
}
