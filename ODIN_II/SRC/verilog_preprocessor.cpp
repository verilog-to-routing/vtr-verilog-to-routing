#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "verilog_preprocessor.h"
#include "types.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include "odin_util.h"
#include <stdbool.h>
#include <regex>

/* Globals */
struct veri_Includes veri_includes;
struct veri_Defines veri_defines;

/* Function declarations */
FILE* open_source_file(char* filename, std::string parent_path);
FILE *remove_comments(FILE *source);
FILE *format_verilog_file(FILE *source);
FILE *format_verilog_variable(FILE * src, FILE *dest);
/*
 * Initialize the preprocessor by allocating sufficient memory and setting sane values
 */
int init_veri_preproc()
{
	veri_includes.included_files = (veri_include **) vtr::calloc(DefaultSize, sizeof(veri_include *));
	if (veri_includes.included_files == NULL)
	{
		perror("veri_includes.included_files : vtr::calloc ");
		return -1;
	}
	veri_includes.current_size = DefaultSize;
	veri_includes.current_index = 0;

	veri_defines.defined_constants = (veri_define **) vtr::calloc(DefaultSize, sizeof(veri_define *));
	if (veri_defines.defined_constants == NULL)
	{
		perror("veri_defines.defined_constants : vtr::calloc ");
		return -1;
	}
	veri_defines.current_size = DefaultSize;
	veri_defines.current_index = 0;
	return 0;
}

/*
 * Cleanup allocated memory
 */
int cleanup_veri_preproc()
{
	//fprintf(stderr, "Cleaning up the verilog preprocessor\n");

	veri_define *def_iterator = veri_defines.defined_constants[0];
	veri_include *inc_iterator = veri_includes.included_files[0];
	int i;

	for (i = 0; i < veri_defines.current_index && i < veri_defines.current_size; def_iterator = veri_defines.defined_constants[++i])
	{
		clean_veri_define(def_iterator);
	}
	def_iterator = NULL;
	veri_defines.current_index = 0;
	veri_defines.current_size = 0;
	vtr::free(veri_defines.defined_constants);

	for (i = 0; i < veri_includes.current_index && i < veri_includes.current_size; inc_iterator = veri_includes.included_files[++i])
	{
		clean_veri_include(inc_iterator);
	}
	inc_iterator = NULL;
	veri_includes.current_index = 0;
	veri_includes.current_size = 0;
	vtr::free(veri_includes.included_files);

	//fprintf(stderr, " --- Finished\n");

	return 0;
}

/*
 * Free memory for a symbol in the define table
 */
void clean_veri_define(veri_define *current)
{
	if (current != NULL)
	{
		//fprintf(stderr, "\tCleaning Symbol: %s, ", current->symbol);
		vtr::free(current->symbol);
		//fprintf(stderr, "Value: %s ", current->value);
		vtr::free(current->value);

		current->defined_in = NULL;

		vtr::free(current);
		current=NULL;
		//fprintf(stderr, "...done\n");
	}
}

/*
 * Free memory for a symbol in the include table
 */
void clean_veri_include(veri_include *current)
{
	if (current != NULL)
	{
		//fprintf(stderr, "\tCleaning Include: %s ", current->path);
		vtr::free(current->path);

		vtr::free(current);
		current = NULL;
		//fprintf(stderr, "...done\n");
	}
}

/*
 * add_veri_define returns a non negative value on success, a -1 if creation of the define failed
 * due to a lack of memory and -2 if the symbol was previously defined and the values conflict
 */
int add_veri_define(char *symbol, char *value, int line, veri_include *defined_in)
{
	int i;
	veri_define *def_iterator = veri_defines.defined_constants[0];
	veri_define *new_def = (veri_define *)vtr::malloc(sizeof(veri_define));
	if (new_def == NULL)
	{
		perror("new_def : vtr::malloc ");
		return -1;
	}

	/* Check to see if there's enough space in our lookup table and reallocate if not. */
	if (veri_defines.current_index == veri_defines.current_size)
	{
		veri_defines.defined_constants = (veri_define **)vtr::realloc(veri_defines.defined_constants, (size_t)(veri_defines.current_size * 2) * sizeof(veri_define *));
		//In a perfect world there is a check here to make sure realloc succeded
		veri_defines.current_size *= 2;
	}

	/* Check previously defined values for collisions. */
	for (i = 0; i < veri_defines.current_index && i < veri_defines.current_size; def_iterator = veri_defines.defined_constants[++i])
	{
		if (0 == strcmp(def_iterator->symbol, symbol))
		{
			fprintf(stderr, "Warning: The constant %s defined on line %d in %s was previously defined on line %d in %s\n",
				symbol, line, defined_in->path, def_iterator->line, def_iterator->defined_in->path);

			if (value == NULL || (value[0] == '/' && value[1] == '/'))
#ifndef BLOCK_EMPTY_DEFINES
			{
				fprintf(stderr, "\tWarning: The new value of %s is empty\n\n", symbol);
				vtr::free(def_iterator->value);
				def_iterator->value =NULL;
			}
#else
			{
				fprintf(stderr, "\tWarning: The new value of %s is empty, doing nothing\n\n", symbol);
				return 0;
			}
#endif
			else if (0 != strcmp(def_iterator->value, value))
			{
				fprintf(stderr, "\tWarning: The value of %s has been redefined to %s, the previous value was %s\n\n",
					symbol, value, def_iterator->value);
				vtr::free(def_iterator->value);
				def_iterator->value = (char *)vtr::strdup(value);
			}

			vtr::free(new_def);
			return -2;
		}
	}

	/* Create the new define and initalize it. */
	new_def->symbol = (char *)vtr::strdup(symbol);
	new_def->value = (value == NULL)? NULL : (char *)vtr::strdup(value);
	new_def->line = line;
	new_def->defined_in = defined_in;

	veri_defines.defined_constants[veri_defines.current_index] = new_def;
	veri_defines.current_index++;

	return 0;
}

/* add_veri_include shall return NULL if it is unable to create a new
 * veri_include in the lookup table or an entry for that file already exists.
 * Otherwise it wil return a pointer to the new veri_include entry.
 */
veri_include* add_veri_include(const char *path, int line, veri_include *included_from)
{
	int i;
	veri_include *inc_iterator = veri_includes.included_files[0];
	veri_include *new_inc = (veri_include *)vtr::malloc(sizeof(veri_include));
	if (new_inc == NULL)
	{
		perror("new_inc : vtr::malloc ");
		return NULL;
	}

	/* Check to see if there's enough space in our lookup table and reallocate if not. */
	if (veri_includes.current_index == veri_includes.current_size)
	{
		veri_includes.included_files = (veri_include **)vtr::realloc(veri_includes.included_files, (size_t)(veri_includes.current_size * 2) * sizeof(veri_include *));
		//In a perfect world there is a check here to make sure realloc succeded
		veri_includes.current_size *= 2;
	}

	/* Scan previous includes to make sure the file wasn't included previously. */
	for (i = 0; i < veri_includes.current_index && i < veri_includes.current_size && inc_iterator != NULL; inc_iterator = veri_includes.included_files[++i])
	{
		if (0 == strcmp(path, inc_iterator->path))
		{
			printf("Warning: including %s multiple times\n", path);
//			vtr::free(new_inc);
//			return NULL;
		}
	}

	new_inc->path = vtr::strdup(path);
	new_inc->included_from = included_from;
	new_inc->line = line;

	veri_includes.included_files[veri_includes.current_index] = new_inc;
	veri_includes.current_index++;

	return new_inc;
}

/*
 * Retrieve the value associated, if any, with the given symbol. If the symbol is not present or no
 * value is associated with the symbol then NULL is returned.
 */
char* ret_veri_definedval(char *symbol)
{
	int is_defined = veri_is_defined(symbol);
	if(0 <= is_defined)
	{
		return veri_defines.defined_constants[is_defined]->value;
	}
	return NULL;

}

/*
 * Returns a non-negative integer if the symbol has been previously defined.
 */
int veri_is_defined(char * symbol)
{
	int i;
	veri_define *def_iterator = veri_defines.defined_constants[0];

	for (i = 0; (i < veri_defines.current_index) && (i < veri_defines.current_size) && (def_iterator != NULL); i++)
	{
		def_iterator = veri_defines.defined_constants[i];
		if (0 == strcmp(symbol, def_iterator->symbol))
		{
			return i;
		}
	}
	return -1;
}

/*
 * Return an open file handle
 * if the file is not in the pwd try the paths indicated by char* list_of_file_names int current_parse_file
 *
 * Return NULL if unable to find and open the file
 */
FILE* open_source_file(char* filename, std::string path)
{
	auto loc = path.find_last_of('/');
	if (loc != std::string::npos) /* No other path to try to find the file */
		path = path.substr(0,loc+1);
	
	path += filename;

	// look in the directory where the file with the include directory resides in
	FILE* src_file = fopen(path.c_str(), "r");
	if (src_file != NULL)
		return src_file;

	// else Look for the file in the PWD as a last resort TODO: this should go away... not standard behavior
	src_file = fopen(filename, "r");
	if (src_file != NULL)
	{
		fprintf(stderr, "Warning: Unable to find %s, opening in current working directory instead\n",
				path.c_str());
		return src_file;
	}



	return NULL;
}

/*
 * Bootstraps our preprocessor
 */
FILE* veri_preproc(FILE *source)
{
	extern global_args_t global_args;
	extern config_t configuration;
	extern size_t current_parse_file;
	FILE *preproc_producer = NULL;

	/* Was going to use filename to prevent duplication but the global var isn't used in the case of a config value */
	veri_include *veri_initial = add_veri_include(configuration.list_of_file_names[current_parse_file].c_str(), 0, NULL);
	if (veri_initial == NULL)
	{
		fprintf(stderr, "Unable to store include information returning original FILE pointer\n\n");
		return source;
	}

	preproc_producer = tmpfile();
	preproc_producer = freopen(NULL, "r+", preproc_producer);

	if (preproc_producer == NULL)
	{
		perror("preproc_producer : fdopen - returning original FILE pointer");
		exit(-1);
		return source;
	}

	/* to thread or not to thread, that is the question. Wether yac will block when waitin */
	fprintf(stderr, "Preprocessing verilog.\n");

	veri_preproc_bootstraped(source, preproc_producer, veri_initial);
	rewind(preproc_producer);
	return preproc_producer;
}

/*
 * Returns a new temporary file with the comments removed.
 * Preserves the line numbers by keeping newlines in place.
 */
FILE *remove_comments(FILE *source)
{
	FILE *destination = tmpfile();
	destination = freopen(NULL, "r+", destination);
	rewind(source);

	char line[MaxLine];
	int in_multiline_comment = FALSE;
	while (fgets(line, MaxLine, source))
	{
		unsigned int i;
		for (i = 0; i < strnlen(line, MaxLine); i++)
		{
			if (!in_multiline_comment)
			{
				// For a single line comment, skip the rest of the line.
				if (line[i] == '/' && line[i+1] == '/')
				{
					break;
				}
				// For a multi-line comment, set the flag and skip over the *.
				else if (line[i] == '/' && line[i+1] == '*')
				{
					i++; // Skip the *.
					in_multiline_comment = TRUE;
				}
				else
				{
					if (line[i] != '\n')
						fputc(line[i], destination);
				}
			}
			else
			{
				// If we're in a multi-line comment, search for the */
				if (line[i] == '*' && line[i+1] == '/')
				{
					i++; // Skip the /
					in_multiline_comment = FALSE;
				}
			}
		}
		fputc('\n', destination);
	}
	rewind(destination);
	return destination;
}

void veri_preproc_bootstraped(FILE *original_source, FILE *preproc_producer, veri_include *current_include)
{
	// Strip the comments from the source file producing a temporary source file.
	FILE *source = remove_comments(original_source);

	source = format_verilog_file(source);

	int line_number = 1;
	veri_flag_stack *skip = (veri_flag_stack *)vtr::calloc(1, sizeof(veri_flag_stack));;
	char line[MaxLine];
	char *token;
	veri_include *new_include = NULL;


	while (NULL != fgets(line, MaxLine, source))
	{
		//fprintf(stderr, "%s:%d\t%s", current_include->path,line_number, line);
		char proc_line[MaxLine] ;
		char symbol[MaxLine] ;
		char *p_proc_line = proc_line ;
		char *last_pch, *pch, *pch_end ;
		// advance past all whitespace
		last_pch = trim(line) ;
		// start searching for backtick
		pch = strchr( last_pch, '`' ) ;
		while ( pch ) {
			// if symbol found, copy everything from end of last_pch to here
			strncpy( p_proc_line, last_pch, pch - last_pch ) ;
			p_proc_line += pch - last_pch ;
			*p_proc_line = '\0' ;

			// find the end of the symbol
			for(pch_end = pch+1 ; pch_end && ( isalnum(*pch_end) || *pch_end == '_' ); pch_end++){}

			// copy symbol into array
			strncpy( symbol, pch+1, pch_end - (pch+1) ) ;
			*(symbol + (pch_end - (pch+1))) = '\0' ;

			char* value = ret_veri_definedval( symbol ) ;
			if ( !value ) {		// symbol not found, just pass it through
				value = symbol;
				*p_proc_line++ = '`' ;
			}
			vtr::strncpy( p_proc_line, value, MaxLine) ;
			p_proc_line += strlen( value ) ;

			last_pch = pch_end ;
			pch = strchr( last_pch+1, '`' ) ;
		}
		vtr::strncpy( p_proc_line, last_pch, MaxLine) ;
		vtr::strncpy( line, proc_line, MaxLine) ;

		//fprintf(stderr, "%s:%d\t%s\n", current_include->path,line_number, line);

		/* Preprocessor directives have a backtick on the first column. */
		if (line[0] == '`')
		{
			token = trim((char *)strtok(line, " \t"));
			//printf("preproc first token: %s\n", token);
			/* If we encounter an `included directive we want to recurse using included_file and
			 * new_include in place of source and current_include
			 */
			if (top(skip) < 1 && strcmp(token, "`include") == 0)
			{
				token = trim((char *)strtok(NULL, "\""));
				FILE *included_file = open_source_file(token,current_include->path);

				/* If we failed to open the included file handle the error */
				if (!included_file)
				{
					fprintf(stderr, "Warning: Unable to open file %s included on line %d of %s\n",
						token, line_number, current_include->path);
					perror("included_file : fopen");
					/*return erro or exit ? */
				}
				else if (NULL != (new_include = add_veri_include(token, line_number, current_include)))
				{
					printf("Including file %s\n", new_include->path);
					veri_preproc_bootstraped(included_file, preproc_producer, new_include);
				}
				fclose(included_file);
				/* If last included file has no newline an error could result so we add one. */
				fputc('\n', preproc_producer);
			}
			/* If we encounter a `define directive we want to add it and its value if any to our
			 * symbol table.
			 */
			else if (top(skip) < 1 && strcmp(token, "`define") == 0)
			{
				char *value = NULL;

				/* strtok is destructive to the original string which we need to retain unchanged, this fixes it. */
				fprintf(preproc_producer, "`define %s\n", line + 1 + strnlen(line, MaxLine));
				//printf("\tIn define: %s", token + 1 + strnlen(token, MaxLine));

				token = trim(strtok(NULL, " \t"));
				//printf("token is: %s\n", token);

				// symbol value can potentially be to the end of the line!
				value = trim(strtok(NULL, "\r\n"));
				//printf("value is: %s\n", value);

				if ( value ) {
					// trim it again just in case
					value = trim(value);
				}
				add_veri_define(token, value, line_number, current_include);
			}
			/* If we encounter a `undef preprocessor directive we want to remove the corresponding
			 * symbol from our lookup table.
			 */
			else if (top(skip) < 1 && strcmp(token, "`undef") == 0)
			{
				int is_defined = 0;
				/* strtok is destructive to the original string which we need to retain unchanged, this fixes it. */
				fprintf(preproc_producer, "`undef %s", line + 1 + strnlen(line, MaxLine));

				token = trim(strtok(NULL, " \t"));

				is_defined = veri_is_defined(token);

				if(is_defined >= 0)
				{
					clean_veri_define(veri_defines.defined_constants[is_defined]);
					veri_defines.defined_constants[is_defined] = veri_defines.defined_constants[veri_defines.current_index];
					veri_defines.defined_constants[veri_defines.current_index--] = NULL;
				}
			}
			else if (strcmp(token, "`ifdef") == 0)
			{
				// if parent is not skipped
				if ( top(skip) < 1 ) {
					int is_defined = 0;

					token = trim(strtok(NULL, " \t"));
					is_defined = veri_is_defined(token);
					if(is_defined < 0) //If we are unable to locate the symbol in the table
					{
						push(skip, 1);
					}
					else
					{
						push(skip, 0);
					}
				}
				// otherwise inherit skip from parent (use 2)
				else {
					push( skip, 2 ) ;
				}
			}
			else if (strcmp(token, "`ifndef") == 0)
			{
				// if parent is not skipped
				if ( top(skip) < 1 ) {
					int is_defined = 0;

					token = trim(strtok(NULL, " \t"));
					is_defined = veri_is_defined(token);
					if(is_defined >= 0) //If we are able to locate the symbol in the table
					{
						push(skip, 1);
					}
					else
					{
						push(skip, 0);
					}
				}
				// otherwise inherit skip from parent (use 2)
				else {
					push( skip, 2 ) ;
				}
			}
			else if (strcmp(token, "`else") == 0)
			{
				// if skip was 0 (prev. ifdef was 1)
				if(top(skip) < 1)
				{
					// then set to 0
					pop(skip) ;
					push(skip, 1);
				}
				// only when prev skip was 1 do we set to 0 now
				else if (top(skip) == 1)
				{
					pop(skip) ;
					push(skip, 0);
				}
				// but if it's 2 (parent ifdef is 1)
				else {
					// then do nothing
				}
			}
			else if (strcmp(token, "`endif") == 0)
			{
				pop(skip);
			}
			/* Leave unhandled preprocessor directives in place. */
			else if (top(skip) < 1)
			{
				fprintf(preproc_producer, "%s %s\n", line, line + 1 + strnlen(line, MaxLine));
			}
		}
		else if(top(skip) < 1)
		{
			if(strnlen(line, MaxLine) <= 0)
			{
				/* There is nothing to print */
			}
			else if( fprintf(preproc_producer, "%s\n", line) < 0)//fputs(line, preproc_producer)
			{
				/* There was an error writing to the stream */
			}
		}
		line_number++;
		token = NULL;
	}
	fclose(source);
	vtr::free(skip);
}

/* General Utility methods ------------------------------------------------- */

/**
 * the trim function remove consequent whitespace and remove trailing whitespace
 */
inline static bool are_both_whitespace(char a, char b) 
{ 
	return isspace(a) && isspace(b);
}

char* trim(char *string)
{
	if (!string)
		return string;

	std::string temp = string;
	//replace large space{1 or more} with single space inside text
	std::string::iterator new_end = std::unique(temp.begin(), temp.end(), are_both_whitespace);
	temp.erase(new_end, temp.end());   

	//remove head whitespace
	if( temp.length() > 0 && isspace(temp[0]) )					temp.erase(0,1);
	//remove trailing whitespace
	if( temp.length() > 0 && isspace(temp[temp.length()-1]) )	temp.erase(temp.length()-1,1);
	//copy string over
	if( temp.length() > 0)										memcpy(string,temp.c_str(), temp.length());
	
	//make sure we have a nul terminated string
	string[temp.length()] = '\0';

	return string;
}

/* ------------------------------------------------------------------------- */



/* stack methods ------------------------------------------------------------*/

int top(veri_flag_stack *stack)
{
	if(stack != NULL && stack->top != NULL)
	{
		return stack->top->flag;
	}
	return 0;

}

int pop(veri_flag_stack *stack)
{
	if(stack != NULL && stack->top != NULL)
	{
		veri_flag_node *top = stack->top;
		int flag = top->flag;

		stack->top = top->next;
		vtr::free(top);

		return flag;
	}
	return 0;
}
void push(veri_flag_stack *stack, int flag)
{
	if(stack != NULL)
	{
		veri_flag_node *new_node = (veri_flag_node *)vtr::malloc(sizeof(veri_flag_node));
		new_node->next = stack->top;
		new_node->flag = flag;

		stack->top = new_node;
	}
}
FILE *format_verilog_file(FILE *source)
{
	FILE *destination = tmpfile();
	char searchString [4][7]= {"input","output","reg","wire"};
	char *line;
	char *line_to_free;
	char ch;
	unsigned i;
	char temp[10];
	int j;
	static const char *pattern = "\\binput\\b|\\boutput\\b|\\breg\\b|\\bwire\\b";
	i = 0;
	line = (char *) calloc (MaxLine, sizeof(char));
	line_to_free = line;
	while( (ch = getc(source) ) != ';')
	{
		line[i++] = ch;
		if (ch == '`')
		{
			while( (ch = getc(source)) != '\n')
			{
				line[i++] = ch;
			}
			line[i] = '\n';
			fputs(line,destination);
			i = 0;
		}

	}
	line[i++] = ch;
	line[i] = '\0';
	line = find_substring(line,"module",2);
	line = search_replace(line,"\n","",2);

	if (! validate_string_regex(line, pattern))
	{
	 	rewind(source);
		return source;
	}
	for (i = 0; i < 4; i++)
	{
		line = search_replace(line,searchString[i],"",2);
	}
	i = 0;
	while (i < strnlen(line, MaxLine))
	{
		if(line[i] == '[')
		{
			j = 0;
			while(line[i] != ']')
			{
				temp[j++] = line[i];
				i++;
			}
			temp[j++] = line[i];
			temp[j] = '\0';
			line = search_replace(line,temp,"",2);
			i = 0;
		}
		else
			i++;
	}
	fputs(line, destination);
	rewind(source);
	destination = format_verilog_variable(source,destination);
	rewind(destination);
	free(line_to_free);
	return destination;
}

FILE *format_verilog_variable(FILE * src, FILE *dest)
{
	char ch;
	int i;
	char line[MaxLine];
	char *tempLine;
	char *pos;
	while( (ch = getc(src) ) != ';')
	{
		if(ch == '(')
		{
			i = 0;
			while( (ch = getc(src) ) != ')')
			{
				if (ch == ',')
				{
					line[i++] = ';';
					line[i] = '\0';
					pos = strstr(line,"reg");
					if (pos != NULL)
					{
						tempLine = search_replace(line,"reg","",2);
						fputs(tempLine,dest);
						tempLine = search_replace(line,"output","",2);
						fputs(tempLine,dest);
					}
					else
					{
						fputs(line, dest);
					}
					i = 0;
				}
				else
					line[i++] = ch;
			}
			line[i-1] = ';';
			line[i] = '\0';
			pos = strstr(line,"reg");
			if (pos != NULL)
			{
				tempLine = search_replace(line,"reg","",2);
				fputs(tempLine,dest);
				tempLine = search_replace(line,"output","",2);
				fputs(tempLine,dest);
			}
			else
			{
				fputs(line, dest);
			}
		}
	}
	while( (ch = getc(src) ) != EOF)
	{
		fputc(ch, dest);
	}
	return dest;
}

/* ------------------------------------------------------------------------- */


