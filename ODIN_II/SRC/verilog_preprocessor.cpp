#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "verilog_preprocessor.h"
#include "odin_types.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include "odin_util.h"
#include <stdbool.h>
#include <regex>
#include <string>

#include "odin_buffer.hpp"

#define DefaultSize 20

/* Structs */
typedef struct veri_include
{
	char *path;
	struct veri_include *included_from;
	int	line;
} veri_include;

typedef struct veri_define
{
	char *symbol;
	char *value;
	int line;
	veri_include *defined_in;
} veri_define;

struct veri_Includes
{
	veri_include **included_files;
	int current_size;
	int current_index;
} ;

struct veri_Defines
{
	veri_define **defined_constants;
	int current_size;
	int current_index;
} ;

/* Stack for tracking conditional branches --------------------------------- */
typedef struct veri_flag_node
{
	int flag;
	struct veri_flag_node *next;
} veri_flag_node;

typedef struct
{
	veri_flag_node *top;
} veri_flag_stack;

void clean_veri_define(veri_define *current);
void clean_veri_include(veri_include *current);

/* Adding/Removing includes or defines */
int add_veri_define(char *symbol, char *value, int line, veri_include *included_from);
char* ret_veri_definedval(const char *symbol);
int veri_is_defined(const char * symbol);
veri_include* add_veri_include(const char *path, int line, veri_include *included_from);

/* Preprocessor ------------------------------------------------------------- */
void veri_preproc_bootstraped(FILE *source, FILE *preproc_producer, veri_include *current_include);

/* stack methods */
int top(veri_flag_stack *stack);
int pop(veri_flag_stack *stack);
void push(veri_flag_stack *stack, int flag);

/* General Utility methods ------------------------------------------------- */
static char* get_line(FILE *source, bool *eof, bool *multiline_comment, const char *one_line_comment, const char *n_line_comment_ST, const char *n_line_comment_END);


/* Globals */
struct veri_Includes veri_includes;
struct veri_Defines veri_defines;

const char symbol_char[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";


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
			warning_message(PARSE_ERROR, -1, -1, "The constant %s defined on line %d in %s was previously defined on line %d in %s\n",
				symbol, line, defined_in->path, def_iterator->line, def_iterator->defined_in->path);

			if (value == NULL || (value[0] == '/' && value[1] == '/'))
#ifndef BLOCK_EMPTY_DEFINES
			{
				warning_message(PARSE_ERROR, -1, -1, "The new value of %s is empty\n\n", symbol);
				vtr::free(def_iterator->value);
				def_iterator->value =NULL;
			}
#else
			{
				warning_message(PARSE_ERROR, -1, -1, "The new value of %s is empty, doing nothing\n\n", symbol);
				return 0;
			}
#endif
			else if (0 != strcmp(def_iterator->value, value))
			{
				warning_message(PARSE_ERROR, -1, -1, "The value of %s has been redefined to %s, the previous value was %s\n\n",
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
	new_def->value = (value == NULL)? vtr::strdup("") : (char *)vtr::strdup(value);
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
		if (0 == strcmp(path, inc_iterator->path))
			warning_message(PARSE_ERROR, line, -1, "Warning: including %s multiple times\n", path);
	

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
char* ret_veri_definedval(const char *symbol)
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
int veri_is_defined(const char * symbol)
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
 * Bootstraps our preprocessor
 */
FILE* veri_preproc(FILE *source)
{
	extern global_args_t global_args;
	extern config_t configuration;
	extern int current_parse_file;
	FILE *preproc_producer = NULL;

	veri_include *veri_initial = add_veri_include(configuration.list_of_file_names[current_parse_file].c_str(), 0, NULL);
	if (veri_initial == NULL)
	{
		fprintf(stderr, "Unable to store include information returning original FILE pointer\n\n");
		return source;
	}

	std::string file_out = 	configuration.debug_output_path 
						+ "/" 
						+ strip_path_and_ext(configuration.list_of_file_names[current_parse_file]).c_str() 
						+ "_preproc.v";

	preproc_producer = fopen(file_out.c_str(), "w+");

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

const char *preproc_symbol_e_STR[] = 
{
	"include",
	"define",
	"undef",
	"ifdef",
	"ifndef",
	"else",
	"endif",
	"preproc_symbol_e_END"
};

typedef enum preproc_symbol_e_e
{
	PREPROC_INCLUDE,
	PREPROC_DEFINE,
	PREPROC_UNDEF,
	PREPROC_IFDEF,
	PREPROC_IFNDEF,
	PREPROC_ELSE,
	PREPROC_ENDIF,
	preproc_symbol_e_END
} preproc_symbol_e;

static preproc_symbol_e get_preproc_directive(const char *in)
{
	preproc_symbol_e to_return = preproc_symbol_e_END;

	if(in && strlen(in) > 0)
	{
		for(int i=0 ; i < (int)preproc_symbol_e_END; i++)
		{
			if(! strcmp(preproc_symbol_e_STR[i], in) )
			{
				to_return = (preproc_symbol_e) i;
				break;
			}
		}
	}
	return to_return;
}

void veri_preproc_bootstraped(FILE *original_source, FILE *preproc_producer, veri_include *current_include)
{

	rewind(original_source);

	int line_number = 1;
	veri_flag_stack *skip = (veri_flag_stack *)vtr::calloc(1, sizeof(veri_flag_stack));

	veri_include *new_include = NULL;

	char *line = NULL;
	buffered_reader_t reader = buffered_reader_t(original_source, "//", "/*", "*/");

	while ((line = reader.get_line()))
	{
		// fprintf(stderr, "%s:%ld\t%s\n", current_include->path,line_number, line);
		if( strlen(line) > 0 )
		{
			std::string proc_line(line);

			// start searching for backticks
			std::size_t pch = proc_line.find_first_of('`') ;

			while ( pch != std::string::npos)
			{			
				std::size_t end_pch = proc_line.find_first_not_of(symbol_char, pch+1);
				if (end_pch != std::string::npos)
				{
					// skip the backtick
					std::string symbol = proc_line.substr(pch+1, (end_pch-pch)-1);

					// verify that this is not a preprocessor directive
					if( get_preproc_directive(symbol.c_str()) == preproc_symbol_e_END)
					{
						// get value from lookup table
						char* value = ret_veri_definedval( symbol.c_str() ) ;
						if (value != NULL)
						{
							proc_line.erase(pch, (end_pch-pch));
							proc_line.insert(pch, value);
						}
					}
				}
							
				// find next backtick
				pch = proc_line.find_first_of('`',  pch+1) ;

			}

			if(! proc_line.empty())
			{
				vtr::free(line);
				line = vtr::strdup(proc_line.c_str());
		
				// fprintf(stderr, "%s:%ld\t%s\n", current_include->path,line_number, line);

				/* Preprocessor directives have a backtick on the first column. */
				if (line[0] == '`')
				{
					char *token = strtok(line, " ");

					// skip the backtick to find the type
					switch( get_preproc_directive(&token[1]) )
					{
						case PREPROC_INCLUDE:
						{
							if(top(skip) < 1)
							{
								/**
								 *  If we encounter an `included directive we want to recurse using included_file and
								 * new_include in place of source and current_include
								 */	

								token = strtok(NULL, "\"");

								std::string current_path = current_include->path;
								auto loc = current_path.find_last_of('/');
								if (loc != std::string::npos) /* No other path to try to find the file */
								{
									current_path = current_path.substr(0,loc+1);
								}
								else
								{
									current_path = "";
								}

								std::string file_path = current_path + token;
								
								FILE* included_file = fopen(file_path.c_str(), "r");
		

								/* If we failed to open the included file handle the error */
								if (!included_file)
								{
									warning_message(PARSE_ERROR, -1, -1, "Unable to open file %s included on line %d of %s\n",
										token, line_number, current_include->path);
									perror("included_file : fopen");
									/*return erro or exit ? */
								}
								else if (NULL != (new_include = add_veri_include(file_path.c_str(), line_number, current_include)))
								{
									printf("Including file %s\n", new_include->path);
									veri_preproc_bootstraped(included_file, preproc_producer, new_include);
								}
								fclose(included_file);
								/* If last included file has no newline an error could result so we add one. */
							}
							break;
						}
						case PREPROC_DEFINE:
						{
							if(top(skip) < 1)
							{
								token = strtok(NULL, " ");
								size_t len = strlen(token);
								char *value = &(token[len+1]);
								
								if(get_preproc_directive(value) != preproc_symbol_e_END)
								{
									printf("Warning! trying to override preprocessor restricted keyword is not allowed\n");
								}
								else
								{
									// symbol value can potentially be to the end of the line!
									add_veri_define(token, value, line_number, current_include);
								}
							}
							break;
						}
						case PREPROC_UNDEF:
						{
							if(top(skip) < 1)
							{
								token = strtok(NULL, " ");
								int is_defined = veri_is_defined(token);
								if(is_defined >= 0)
								{
									clean_veri_define(veri_defines.defined_constants[is_defined]);
									veri_defines.defined_constants[is_defined] = veri_defines.defined_constants[veri_defines.current_index];
									veri_defines.defined_constants[veri_defines.current_index--] = NULL;
								}
							}
							break;
						}
						case PREPROC_IFDEF:
						{
							// if parent is not skipped
							if ( top(skip) < 1 ) 
							{
								token = strtok(NULL, " ");
								int is_defined = veri_is_defined(token);
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
							else 
							{
								push( skip, 2 ) ;
							}
							break;
						}
						case PREPROC_IFNDEF:
						{
							// if parent is not skipped
							if ( top(skip) < 1 ) 
							{
								token = strtok(NULL, " ");
								int is_defined = veri_is_defined(token);
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
							else 
							{
								push( skip, 2 ) ;
							}
							break;
						}
						case PREPROC_ELSE:
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
							else 
							{
								// then do nothing
							}
							break;
						}
						case PREPROC_ENDIF:
						{
							pop(skip);
							break;
						}
						default:
						{
							/* Leave unhandled preprocessor directives in place. */
							if (top(skip) < 1)
							{
								fprintf(preproc_producer, "%s %s", line, line + 1 + strlen(line));
							}

							break;
						}
					}
				}
				else if(top(skip) < 1)
				{
					fprintf(preproc_producer, "%s", line);							
				}
			}
		}

		fprintf(preproc_producer,"\n");
		line_number++;
		vtr::free(line);
	}
	vtr::free(skip);
}

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
