#ifndef ODIN_UTIL_H
#define ODIN_UTIL_H

#include <string>

#include "odin_types.h"

#define MAX_BUF 2048

long shift_left_value_with_overflow_check(long input_value, long shift_by, loc_t loc);

std::string get_file_extension(std::string input_file);
std::string get_directory(std::string input_file);
void create_directory(std::string path);
void assert_supported_file_extension(std::string input_file, loc_t loc);
FILE* open_file(const char* file_name, const char* open_type);
void get_current_path();

const char* name_based_on_op(operation_list op);
const char* name_based_on_ids(ids op);
const char* node_name_based_on_op(nnode_t* node);
const char* ast_node_name_based_on_ids(ast_node_t* node);

char* make_signal_name(char* signal_name, int bit);
char* make_full_ref_name(const char* previous, const char* module_name, const char* module_instance_name, const char* signal_name, long bit);
char* make_full_name_w_o_array_ref(const char* previous, const char* module_name, const char* module_instance_name);
bool output_vector_headers_equal(char* buffer1, char* buffer2);

char* twos_complement(char* str);
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

char* make_string_based_on_id(nnode_t* node);
std::string make_simple_name(char* input, const char* flatten_string, char flatten_char);

void* my_malloc_struct(long bytes_to_alloc);

void reverse_string(char* token, int length);
char* append_string(const char* string, const char* appendage, ...);
void string_to_upper(char* string);
void string_to_lower(char* string);

int is_binary_string(char* string);
int is_octal_string(char* string);
int is_decimal_string(char* string);
int is_hex_string(char* string);
int is_dont_care_string(char* string);

char* get_pin_name(char* name);
char* get_port_name(char* name);
char* get_hard_block_node_name(char* name);
char* get_stripped_name(const char* subcircuit_name);
char* retrieve_node_type_from_subckt_name(const char* subcircuit_name);
int get_pin_number(char* name);
short get_bit(char in);
short get_bit(short in);
char* search_replace(char* src, const char* sKey, const char* rKey, int flag);
std::string find_substring(char* src, const char* sKey, int flag);

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
