#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "globals.h"
#include "types.h"
#include "errors.h"
#include "ast_util.h"
#include "odin_util.h"
#include "util.h"
#include "high_level_data.h"

/*---------------------------------------------------------------------------------------------*/
void update_tree(ast_node_t *node);
void update_tag(ast_node_t *node, int tag, int line);
int generate_tag();
int last_tag;
int get_linenumber(ast_node_t *node);
/*---------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * (function: add_tag_data)
 *-------------------------------------------------------------------------*/
void add_tag_data()
{
	int i;

	for (i = 0; i < num_modules; i++)
	{
		update_tree (ast_modules[i]);
	}
}

/*---------------------------------------------------------------------------
 * (function: update_tree)
 *-------------------------------------------------------------------------*/
void update_tree(ast_node_t *node)
{
	int i;
	int tag;
	int line;

	if (node == NULL) 
		return;
	
	if (strcmp(global_args.high_level_block,"if")==0)
	{
		switch(node->type)
		{
			case IF:
				tag = generate_tag();
				line = get_linenumber(node);
				update_tag(node, tag, line);
				break;
			
			default:
				for (i=0; i < node->num_children; i++)
				{
					update_tree(node->children[i]);
				}
		}
	}
	else if (strcmp(global_args.high_level_block,"always")==0)
	{
		switch(node->type)
		{
			case ALWAYS:
				tag = generate_tag();
				line = get_linenumber(node);
				update_tag(node, tag, line);
				break;
			
			default:
				for (i=0; i < node->num_children; i++)
				{
					update_tree(node->children[i]);
				}
		}
	}
	else if (strcmp(global_args.high_level_block,"module")==0)
	{
		switch(node->type)
		{
			case MODULE:
				tag = generate_tag();
				line = get_linenumber(node);
				update_tag(node, tag, line);
				break;
			
			default:
				for (i=0; i < node->num_children; i++)
				{
					update_tree(node->children[i]);
				}
		}
	}
}

/*---------------------------------------------------------------------------
 * (function: update_tag)
 *-------------------------------------------------------------------------*/
void update_tag(ast_node_t *node, int tag, int line)
{
	int i;
	if (node == NULL) 
		return;

	node->far_tag = tag;
	node->high_number = line;

	for (i=0; i < node->num_children; i++)
	{
		update_tag(node->children[i],tag, line);
	}
	
}

/*---------------------------------------------------------------------------
 * (function: generate_tag)
 *-------------------------------------------------------------------------*/
int generate_tag()
{
	static int high_level_id = 0;
	high_level_id ++;
	last_tag = high_level_id;

	return high_level_id;
}

/*---------------------------------------------------------------------------
 * (function: get_linenumber)
 *-------------------------------------------------------------------------*/
int get_linenumber(ast_node_t *node)
{
	return node->line_number;
}
