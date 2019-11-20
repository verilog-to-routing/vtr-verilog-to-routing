#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "hash.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "ReadLine.h"
#include "globals.h"


static char *pinlist_str = "pinlist:";
enum special_blk
{ NORMAL = 0, INPAD, OUTPAD };

static t_type_ptr get_type_by_name(IN const char *name,
				   IN int num_types,
				   IN const struct s_type_descriptor
				   block_types[],
				   IN t_type_ptr IO_type,
				   IN const char *net_file,
				   IN int line,
				   OUT enum special_blk *overide);



/* Initializes the block_list with info from a netlist */
void
read_netlist(IN const char *net_file,
	     IN int num_types,
	     IN const struct s_type_descriptor block_types[],
	     IN t_type_ptr IO_type,
	     IN int io_ipin,
	     IN int io_opin,
	     OUT t_subblock_data * subblock_data_ptr,
	     OUT int *num_blocks,
	     OUT struct s_block *block_list[],
	     OUT int *num_nets,
	     OUT struct s_net *net_list[])
{

    FILE *infile;
    int bcount;
    struct s_block *blist;
    int ncount;
    struct s_net *nlist;

    int i, j, k, l, m;
    enum
    { COUNT, LOAD, MAP, STOP }
    pass;
    int line, prev_line;
    enum special_blk overide;
    char **block_tokens;
    char **pin_tokens;
    char **tokens;
    t_subblock **slist = NULL;
    int *scount = NULL;
    t_type_ptr type = NULL;


    infile = my_fopen(net_file, "r");
    bcount = 0;
    blist = NULL;
    ncount = 0;
    nlist = NULL;
    memset(subblock_data_ptr, 0, sizeof(t_subblock_data));

    /* Multi-pass load 
     * COUNT
     * -> count blocks
     * -> count nets
     * 
     * LOAD
     * -> alloc num_subblocks_per_block list
     * -> allocate s_block list at start
     * -> allocate s_net list at start
     * -> count subblocks per block
     * -> sets block names
     * -> sets block types
     * -> allocs block net lists
     * -> sets net names
     * 
     * MAP
     * -> fills in s_block:nets list 
     * -> fills in subblocks */

    for(pass = 0; pass < STOP; ++pass)
	{
	    rewind(infile);
	    line = 0;
	    i = 0;
	    j = 0;
	    overide = NORMAL;

	    /* Alloc the lists */
	    if(LOAD == pass)
		{
		    blist =
			(struct s_block *)my_malloc(sizeof(struct s_block) *
						    bcount);
		    memset(blist, 0, (sizeof(struct s_block) * bcount));

		    nlist =
			(struct s_net *)my_malloc(sizeof(struct s_net) *
						  ncount);
		    memset(nlist, 0, (sizeof(struct s_net) * ncount));

		    slist =
			(t_subblock **) my_malloc(sizeof(t_subblock *) *
						  bcount);
		    memset(slist, 0, (sizeof(t_subblock *) * bcount));

		    scount = (int *)my_malloc(sizeof(int) * bcount);
		    memset(scount, 0, (sizeof(int) * bcount));
		}

	    /* Read file line by line */
	    block_tokens = ReadLineTokens(infile, &line);
	    prev_line = line;
	    while(block_tokens)
		{

		    /* .global directives have special meaning */
		    if(0 == strcmp(block_tokens[0], ".global"))
			{
			    if(MAP == pass)
				{
				    for(l = 0; l < ncount; ++l)
					{
					    if(0 ==
					       strcmp(nlist[l].name,
						      block_tokens[1]))
						{
						    nlist[l].is_global = TRUE;
						    break;
						}
					}
				    if(l == ncount)
					{
					    printf(ERRTAG
						   "'%s':%d - '.global' specified an invalid net\n",
						   net_file, prev_line);
					    exit(1);
					}
				}

			    /* Don't do any more processing on this */
			    FreeTokens(&block_tokens);
			    block_tokens = ReadLineTokens(infile, &line);
			    prev_line = line;
			    continue;
			}
		    pin_tokens = ReadLineTokens(infile, &line);

		    if(CountTokens(block_tokens) != 2)
			{
			    printf(ERRTAG "'%s':%d - block type line should "
				   "be in form '.type_name block_name'\n",
				   net_file, prev_line);
			    exit(1);
			}
		    if(NULL == pin_tokens)
			{
			    printf(ERRTAG
				   "'%s':%d - blocks must be follow by a 'pinlist:' line\n",
				   net_file, line);
			    exit(1);
			}
		    if(0 != strcmp("pinlist:", pin_tokens[0]))
			{
			    printf(ERRTAG
				   "'%s':%d - 'pinlist:' line must follow "
				   "block type line\n", net_file, line);
			    exit(1);
			}

		    type =
			get_type_by_name(block_tokens[0], num_types,
					 block_types, IO_type, net_file,
					 prev_line, &overide);
		    /* Check if we are overiding the pinlist format for this block */
		    if(overide)
			{
			    if(CountTokens(pin_tokens) != 2)
				{	/* 'pinlist:' and name */
				    printf(ERRTAG
					   "'%s':%d - pinlist for .input and .output should "
					   "only have one item.\n", net_file,
					   line);
				    exit(1);
				}

			    /* Make a new faked token list with 'pinlist:' and then pin mappings and a null */
			    tokens =
				(char **)my_malloc(sizeof(char *) *
						   (type->num_pins + 2));

			    l = strlen(pinlist_str) + 1;
			    k = strlen(pin_tokens[1]) + 1;
			    tokens[0] =
				(char *)my_malloc(sizeof(char) * (k + l));
			    memcpy(tokens[0], pinlist_str, l * sizeof(char));
			    memcpy(tokens[0] + l, pin_tokens[1],
				   k * sizeof(char));

			    /* Set all other pins to open */
			    for(k = 0; k < type->num_pins; ++k)
				{
				    tokens[1 + k] = "open";	/* free wont be called on this so is safe */
				}
			    tokens[1 + k] = NULL;	/* End of token list marker */
			    /* Set the one pin with the value given */
			    for(k = 0; k < type->num_pins; ++k)
				{
				    switch (overide)
					{
					case INPAD:
					    tokens[1 + io_opin] =
						(tokens[0] + l);
					    break;
					case OUTPAD:
					    tokens[1 + io_ipin] =
						(tokens[0] + l);
					    break;
					}
				}
			    FreeTokens(&pin_tokens);
			    pin_tokens = tokens;
			    tokens = NULL;
			}

		    if(CountTokens(pin_tokens) != (type->num_pins + 1))
			{
			    printf(ERRTAG
				   "'%s':%d - 'pinlist:' line has %d pins instead of "
				   "expect %d pins.\n", net_file, line,
				   CountTokens(pin_tokens) - 1,
				   type->num_pins);
			    exit(1);
			}

		    /* Load block name and type and alloc net list */
		    if(LOAD == pass)
			{
			    blist[i].name = my_strdup(block_tokens[1]);
			    blist[i].type = type;
			    blist[i].nets =
				(int *)my_malloc(sizeof(int) *
						 type->num_pins);
			    for(k = 0; k < type->num_pins; ++k)
				{
				    blist[i].nets[k] = OPEN;
				}
			}

		    /* Examine pin list to determine nets */
		    for(k = 0; k < type->num_pins; ++k)
			{
			    if(0 != strcmp("open", pin_tokens[1 + k]))
				{
				    if(DRIVER ==
				       type->class_inf[type->pin_class[k]].
				       type)
					{
					    if(LOAD == pass)
						{
						    nlist[j].name =
							my_strdup(pin_tokens
								  [1 + k]);
						}
					    if(MAP == pass)
						{
						    blist[i].nets[k] = j;	/* If we are net source we don't need to search */
						}

					    ++j;	/* This was an active netlist */
					}
				    else
					{
					    if(MAP == pass)
						{
						    /* Map sinks by doing a linear search to find the net */
						    blist[i].nets[k] = OPEN;
						    for(l = 0; l < ncount;
							++l)
							{
							    if(0 ==
							       strcmp(nlist
								      [l].
								      name,
								      pin_tokens
								      [1 +
								       k]))
								{
								    blist[i].
									nets
									[k] =
									l;
								    break;
								}
							}
						    if(OPEN ==
						       blist[i].nets[k])
							{
							    printf(ERRTAG
								   "'%s':%d - Net '%s' not found\n",
								   net_file,
								   line,
								   pin_tokens
								   [1 + k]);
							    exit(1);
							}
						}
					}
				}
			}

		    /* Allocating subblocks */
		    if(MAP == pass)
			{
				/* All blocks internally have subblocks but I/O subblocks are
				allocated and loaded elsewhere */
				if(scount[i] > 0) {
					slist[i] = (t_subblock *)
					my_malloc(sizeof(t_subblock) * scount[i]);
					for(k = 0; k < scount[i]; ++k)
					{
						slist[i][k].name = NULL;
						slist[i][k].clock = OPEN;
						slist[i][k].inputs =
						(int *)my_malloc(sizeof(int) *
								 type->
								 max_subblock_inputs);
						for(l = 0; l < type->max_subblock_inputs;
						++l)
						{
							slist[i][k].inputs[l] = OPEN;
						}
						slist[i][k].outputs =
						(int *)my_malloc(sizeof(int) *
								 type->
								 max_subblock_outputs);
						for(l = 0; l < type->max_subblock_outputs;
						++l)
						{
							slist[i][k].outputs[l] = OPEN;
						}
					}
				}
			}

		    /* Ignore subblock data */
		    tokens = ReadLineTokens(infile, &line);
		    prev_line = line;
		    m = 0;
		    while(tokens && (0 == strcmp(tokens[0], "subblock:")))
			{
			    if(CountTokens(tokens) != (type->max_subblock_inputs + type->max_subblock_outputs + 1 +	/* clocks */
						       2))
				{	/* 'subblock:', name */
				    printf("subblock wrong pin count, netlist has %d, architecture as %d on line %d \n" ,
						CountTokens(tokens) - 2, (type->max_subblock_inputs + type->max_subblock_outputs + 1),
						line);
				    exit(1);
				}

			    /* Count subblocks given */
			    if(LOAD == pass)
				{
				    scount[i]++;
				}

			    /* Load subblock name */
			    if(MAP == pass)
				{
				    assert(i < bcount);
				    assert(m < scount[i]);
				    slist[i][m].name = my_strdup(tokens[1]);
				    for(k = 0; k < type->max_subblock_inputs;
					++k)
					{
					    /* Check prefix and load pin num */
					    l = 2 + k;
					    if(0 ==
					       strncmp("ble_", tokens[l], 4))
						{
						    slist[i][m].inputs[k] = type->num_pins + my_atoi(tokens[l] + 4);	/* Skip the 'ble_' part */
						}
					    else if(0 !=
						    strcmp("open", tokens[l]))
						{
						    slist[i][m].inputs[k] =
							my_atoi(tokens[l]);
						}
					}
				    for(k = 0; k < type->max_subblock_outputs;
					++k)
					{
					    l = 2 +
						type->max_subblock_inputs + k;
					    if(0 != strcmp("open", tokens[l]))
						{
						    slist[i][m].outputs[k] =
							my_atoi(tokens[l]);
						}
					}
				    l = 2 + type->max_subblock_inputs +
					type->max_subblock_outputs;
				    if(0 != strcmp("open", tokens[l]))
					{
					    slist[i][m].clock =
						my_atoi(tokens[l]);
					}
				}

			    ++m;	/* Next subblock */

			    FreeTokens(&tokens);
			    tokens = ReadLineTokens(infile, &line);
			    prev_line = line;
			}

		    if(pass > COUNT)
			{
			    /* Check num of subblocks read */
			    if(scount[i] > type->max_subblocks)
				{
					printf("too many subblocks on block [%d] %s\n", i, blist[i].name);
				    exit(1);
				}
			}

		    ++i;	/* End of this block */

		    FreeTokens(&block_tokens);
		    FreeTokens(&pin_tokens);
		    block_tokens = tokens;
		}

	    /* Save counts */
	    if(COUNT == pass)
		{
		    bcount = i;
		    ncount = j;
		}
	}
    fclose(infile);

    /* Builds mappings from each netlist to the blocks contained */
    sync_nets_to_blocks(bcount, blist, ncount, nlist);

    /* Send values back to caller */
    *num_blocks = bcount;
    *block_list = blist;
    *num_nets = ncount;
    *net_list = nlist;
    subblock_data_ptr->subblock_inf = slist;
    subblock_data_ptr->num_subblocks_per_block = scount;
}


static t_type_ptr
get_type_by_name(IN const char *name,
		 IN int num_types,
		 IN const struct s_type_descriptor block_types[],
		 IN t_type_ptr IO_type,
		 IN const char *net_file,
		 IN int line,
		 OUT enum special_blk *overide)
{

    /* Just does a simple linear search for now */
    int i;

    /* pin_overide is used to specify that the .input and .output
     * blocks only have one pin specified and should be treated 
     * as if used as a .io with a full pin spec */
    *overide = NORMAL;

    /* .input and .output are special names that map to the basic IO type */
    if(0 == strcmp(".input", name))
	{
	    *overide = INPAD;
	    return IO_type;
	}
    if(0 == strcmp(".output", name))
	{
	    *overide = OUTPAD;
	    return IO_type;
	}

    /* Linear type search. Don't really expect to have too many 
     * types for it to be a problem */
    for(i = 0; i < num_types; ++i)
	{
	    if(0 == strcmp(block_types[i].name, name))
		{
		    return (block_types + i);
		}
	}

    /* No type matched */
    printf(ERRTAG "'%s':%d - Invalid block type '%s'\n",
	   net_file, line, name);
    exit(1);
}
