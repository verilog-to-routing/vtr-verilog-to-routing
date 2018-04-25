#ifndef verilog_preprocessor_h
#define verilog_preprocessor_h

#define DefaultSize 20
#define MaxLine	4096

//#define BLOCK_EMPTY_DEFINES

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

/* Globals */
extern struct veri_Includes veri_includes;
extern struct veri_Defines veri_defines;

/* Initalization and Cleanup */
int init_veri_preproc();
int cleanup_veri_preproc();
void clean_veri_define(veri_define *current);
void clean_veri_include(veri_include *current);

/* Adding/Removing includes or defines */
int add_veri_define(char *symbol, char *value, int line, veri_include *included_from);
char* ret_veri_definedval(char *symbol);
int veri_is_defined(char * symbol);
veri_include* add_veri_include(char *path, int line, veri_include *included_from);

/* Preprocessor ------------------------------------------------------------- */
FILE* veri_preproc(FILE *source);
void veri_preproc_bootstraped(FILE *source, FILE *preproc_producer, veri_include *current_include);

/* ------------------------------------------------------------------------- */


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

/* stack methods */
int top(veri_flag_stack *stack);
int pop(veri_flag_stack *stack);
void push(veri_flag_stack *stack, int flag);

/* ------------------------------------------------------------------------- */


/* General Utility methods ------------------------------------------------- */
char* trim(char *string);

/* ------------------------------------------------------------------------- */


#endif
