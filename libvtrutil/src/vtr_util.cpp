#include <cstdarg>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_error.h"


namespace vtr {

constexpr size_t BUFSIZE = 32768; /* Maximum line length for various parsing proc. */

char *out_file_prefix = NULL; /* used by fopen */
static int file_line_number = 0; /* file in line number being parsed (used by fgets) */
static int cont; /* line continued? (used by strtok)*/


//Splits the string 'text' along the specified delimiter characters in 'delims'
//The split strings (excluding the delimiters) are returned
std::vector<std::string> split(const std::string& text, const std::string delims) {
    std::vector<std::string> tokens;

    std::string curr_tok;
    for(char c : text) {
        if(delims.find(c) != std::string::npos) {
            //Delimeter character
            if(!curr_tok.empty()) {
                //At the end of the token

                //Save it
                tokens.push_back(curr_tok);

                //Reset token
                curr_tok.clear();
            } else {
                //Pass
            }
        } else {
            //Non-delimeter append to token
            curr_tok += c;
        }
    }
    
    //Add last token
    if(!curr_tok.empty()) {
        //Save it
        tokens.push_back(curr_tok);
    } 
    return tokens;
}

//Returns a std::string formatted using a printf-style format string
std::string string_fmt(const char* fmt, ...) {
	// Make a variable argument list
	va_list va_args;

	// Initialize variable argument list
	va_start(va_args, fmt);

    //Format string
    std::string str = vstring_fmt(fmt, va_args);

	// Reset variable argument list
	va_end(va_args);

    return str;
}

//Returns a std::string formatted using a printf-style format string taking
//an explicit va_list
std::string vstring_fmt(const char* fmt, va_list args) {
    
    //Determine the formatted length
    int len = snprintf(nullptr, 0, fmt, args); 

    //Negative if there is a problem with the format string
    assert(len >= 0 && "Problem decoding format string");

    size_t buf_size = len + 1; //For terminator

    //Allocate a buffer
    //  unique_ptr will free buffer automatically
    std::unique_ptr<char[]> buf(new char[buf_size]);

    //Format into the buffer
    len = snprintf(buf.get(), buf_size, fmt, args);

    assert(len >= 0 && "Problem decoding format string");
    assert(static_cast<size_t>(len) == buf_size - 1);

    //Build the string from the buffer
    return std::string(buf.get(), len);
}

/* An alternate for strncpy since strncpy doesn't work as most
 * people would expect. This ensures null termination */
char* strncpy(char *dest, const char *src, size_t size) {
	/* Find string's length */
	size_t len = strlen(src);

	/* Cap length at (num - 1) to leave room for \0 */
	if (size <= len)
		len = (size - 1);

	/* Copy as much of string as we can fit */
	memcpy(dest, src, len);

	/* explicit null termination */
	dest[len] = '\0';

	return dest;
}

char* strdup(const char *str) {
	size_t Len;
	char *Dst;

	if (str == NULL ) {
		return NULL ;
	}

	Len = 1 + strlen(str);
	Dst = (char *) my_malloc(Len * sizeof(char));
	memcpy(Dst, str, Len);

	return Dst;
}

int atoi(const char *str) {

	/* Returns the integer represented by the first part of the character       *
	 * string.                                              */

	if (str[0] < '0' || str[0] > '9') {
		if (!(str[0] == '-' && str[1] >= '0' && str[1] <= '9')) {
            throw VtrError(string_fmt("Expected integer instead of '%s'", str), __FILE__, __LINE__);
		}
	}
	return (atoi(str));
}

char* strtok(char *ptr, const char *tokens, FILE * fp, char *buf) {

	/* Get next token, and wrap to next line if \ at end of line.    *
	 * There is a bit of a "gotcha" in strtok.  It does not make a   *
	 * copy of the character array which you pass by pointer on the  *
	 * first call.  Thus, you must make sure this array exists for   *
	 * as long as you are using strtok to parse that line.  Don't    *
	 * use local buffers in a bunch of subroutines calling each      *
	 * other; the local buffer may be overwritten when the stack is  *
	 * restored after return from the subroutine.                    */

	char *val;

	val = std::strtok(ptr, tokens);
	for (;;) {
		if (val != NULL || cont == 0)
			return (val);

		/* return unless we have a null value and a continuation line */
		if (vtr::fgets(buf, BUFSIZE, fp) == NULL )
			return (NULL );

		val = std::strtok(buf, tokens);
	}
}

FILE* fopen(const char *fname, const char *flag) {
	FILE *fp;
	size_t Len;
	char *new_fname = NULL;
	file_line_number = 0;

	/* Appends a prefix string for output files */
	if (out_file_prefix) {
		if (strchr(flag, 'w')) {
			Len = 1; /* NULL char */
			Len += strlen(out_file_prefix);
			Len += strlen(fname);
			new_fname = (char *) my_malloc(Len * sizeof(char));
			strcpy(new_fname, out_file_prefix);
			strcat(new_fname, fname);
			fname = new_fname;
		}
	}

	if (NULL == (fp = std::fopen(fname, flag))) {
        throw VtrError(string_fmt("Error opening file %s for %s access: %s.\n", fname, flag, strerror(errno)), __FILE__, __LINE__);		
	}

	if (new_fname)
		free(new_fname);

	return (fp);
}

char* fgets(char *buf, int max_size, FILE * fp) {
	/* Get an input line, update the line number and cut off *
	 * any comment part.  A \ at the end of a line with no   *
	 * comment part (#) means continue. vtr::fgets should give * 
	 * identical results for Windows (\r\n) and Linux (\n)   *
	 * newlines, since it replaces each carriage return \r   *
	 * by a newline character \n.  Returns NULL after EOF.	 */

	char ch;
	int i;

	cont = 0; /* line continued? */
	file_line_number++; /* global variable */

	for (i = 0; i < max_size - 1; i++) { /* Keep going until the line finishes or the buffer is full */

		ch = fgetc(fp);

		if (feof(fp)) { /* end of file */
			if (i == 0) {
				return NULL ; /* required so we can write while (vtr::fgets(...) != NULL) */
			} else { /* no newline before end of file - last line must be returned */
				buf[i] = '\0';
				return buf;
			}
		}

		if (ch == '#') { /* comment */
			buf[i] = '\0';
			while ((ch = fgetc(fp)) != '\n' && !feof(fp))
				; /* skip the rest of the line */
			return buf;
		}

		if (ch == '\r' || ch == '\n') { /* newline (cross-platform) */
			if (i != 0 && buf[i - 1] == '\\') { /* if \ at end of line, line continued */
				cont = 1;
				buf[i - 1] = '\n'; /* May need this for tokens */
				buf[i] = '\0';
			} else {
				buf[i] = '\n';
				buf[i + 1] = '\0';
			}
			return buf;
		}

		buf[i] = ch; /* copy character into the buffer */

	}

	/* Buffer is full but line has not terminated, so error */
    throw VtrError(string_fmt("Error on line %d -- line is too long for input buffer.\n"
                              "All lines must be at most %d characters long.\n", BUFSIZE - 2),
                    __FILE__, __LINE__);
	return NULL;
}

/*
 * Returns line number of last opened and read file
 */
int get_file_line_number_of_last_opened_file() {
	return file_line_number;
}


bool file_exists(const char* filename) {
	FILE * file;

	if (filename == NULL ) {
		return false;
	}

	file = fopen(filename, "r");
	if (file) {
		fclose(file);
		return true;
	}
	return false;
}

/* Date:July 17th, 2013								
 * Author: Daniel Chen								
 * Purpose: Checks the file extension of an file to ensure 
 *			correct file format. Returns true if format is 
 *			correct, and false otherwise.
 * Note:	This is probably a fragile check, but at least 
 *			should prevent common problems such as swapping
 *			architecture file and blif file on the VPR 
 *			command line. 
 */

bool check_file_name_extension(const char* file_name, 
							   const char* file_extension){
	const char* str;
	int len_extension;

	len_extension = strlen(file_extension);
	str = strstr(file_name, file_extension);
	if(str == NULL || (*(str + len_extension) != '\0')){
		return false;
	}

	return true;
}

} //namespace
