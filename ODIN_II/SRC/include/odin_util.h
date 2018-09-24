#ifndef ODIN_UTIL_H
#define ODIN_UTIL_H
#include "types.h"

#define MAX_BUF 256

char *make_signal_name(char *signal_name, int bit);
char *make_full_ref_name(const char *previous, char *module_name, char *module_instance_name, const char *signal_name, long bit);

char *twos_complement(char *str);
int is_string_of_radix(char *string, int radix);
char *convert_string_of_radix_to_bit_string(char *string, int radix, int binary_size);
long long convert_string_of_radix_to_long_long(char *orig_string, int radix);
char *convert_long_long_to_bit_string(long long orig_long, int num_bits);
long long convert_dec_string_of_size_to_long_long(char *orig_string, int size);
char *convert_hex_string_of_size_to_bit_string(short is_dont_care_number, char *orig_string, int size);
char *convert_oct_string_of_size_to_bit_string(char *orig_string, int size);
char *convert_binary_string_of_size_to_bit_string(short is_dont_care_number, char *orig_string, int binary_size);

long long int my_power(long long int x, long long int y);
long long int pow2(int to_the_power);

char *make_string_based_on_id(nnode_t *node);
std::string make_simple_name(char *input, const char *flatten_string, char flatten_char);

void *my_malloc_struct(size_t bytes_to_alloc);

void reverse_string(char *token, int length);
char *append_string(const char *string, const char *appendage, ...);
void string_to_upper(char *string);
void string_to_lower(char *string);

int is_binary_string(char *string);
int is_octal_string(char *string);
int is_decimal_string(char *string);
int is_hex_string(char *string);
int is_dont_care_string(char *string);

char *get_pin_name  (char *name);
char *get_port_name (char *name);
int get_pin_number  (char *name);
short get_bit(char in);
char *search_replace(char *src, const char *sKey, const char *rKey, int flag);
bool validate_string_regex(const char *str, const char *pattern);
char *find_substring(char *src,const char *sKey,int flag);


void print_time(double time);
double wall_time();
std::string strip_path_and_ext(std::string file);
std::vector<std::string> parse_seperated_list(char *list, const char *separator);

int print_progress_bar(double completion, int position, int length, double time);

void trim_string(char* string, const char *chars);
bool only_one_is_true(std::vector<bool> tested);
int odin_sprintf (char *s, const char *format, ...);


#endif

#ifndef ERRORS_H
#define ERRORS_H

void error_message(short error_type, int line_number, int file, const char *message, ...);
void warning_message(short error_type, int line_number, int file, const char *message, ...);

#endif
