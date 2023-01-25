/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
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

#ifndef ODIN_UTIL_H
#define ODIN_UTIL_H

#include <string>

#include "odin_types.h"

#define MAX_BUF 2048

long shift_left_value_with_overflow_check(long input_value, long shift_by, loc_t loc);

std::string get_file_extension(std::string input_file);
std::string get_directory(std::string input_file);
void create_directory(std::string path);
void report_frontend_elaborator();
void assert_supported_file_extension(std::string input_file, loc_t loc);
FILE* open_file(const char* file_name, const char* open_type);
void get_current_path();

const char* name_based_on_op(operation_list op);
const char* name_based_on_ids(ids op);
const char* node_name_based_on_op(nnode_t* node);
const char* ast_node_name_based_on_ids(ast_node_t* node);

char* make_signal_name(char* signal_name, int bit);
char* make_full_ref_name(const char* previous, const char* module_name, const char* module_instance_name, const char* signal_name, long bit);
bool output_vector_headers_equal(char* buffer1, char* buffer2);

int is_string_of_radix(char* string, int radix);
char* convert_string_of_radix_to_bit_string(char* string, int radix, int binary_size);
long convert_string_of_radix_to_long(char* orig_string, int radix);
char* convert_long_to_bit_string(long orig_long, int num_bits);
long convert_dec_string_of_size_to_long(char* orig_string, int size);
char* convert_hex_string_of_size_to_bit_string(short is_dont_care_number, char* orig_string, int size);
char* convert_oct_string_of_size_to_bit_string(char* orig_string, int size);
char* convert_binary_string_of_size_to_bit_string(short is_dont_care_number, char* orig_string, int binary_size);

long int my_power(long int x, long int y);
long int pow2(int to_the_power);

std::string make_simple_name(char* input, const char* flatten_string, char flatten_char);

void* my_malloc_struct(long bytes_to_alloc);

void reverse_string(char* token, int length);
char* append_string(const char* string, const char* appendage, ...);
char* string_to_upper(char* string);
std::string string_to_lower(std::string string);

int is_binary_string(char* string);
int is_octal_string(char* string);
int is_decimal_string(char* string);
int is_hex_string(char* string);
int is_dont_care_string(char* string);

char* get_pin_name(char* name);
char* get_port_name(char* name);
char* get_stripped_name(const char* subcircuit_name);
int get_pin_number(char* name);

void print_time(double time);
double wall_time();
int print_progress_bar(double completion, int position, int length, double time);

void trim_string(char* string, const char* chars);
bool only_one_is_true(std::vector<bool> tested);
int odin_sprintf(char* s, const char* format, ...);
char* str_collate(char* str1, char* str2);

void passed_verify_i_o_availabilty(nnode_t* node, int expected_input_size, int expected_output_size, const char* current_src, int line_src);

void print_input_files_info();

#endif
