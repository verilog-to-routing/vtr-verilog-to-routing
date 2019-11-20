#include <string.h>
#include <assert.h>
#include "util.h"
#include "vpr_types.h"
#include "ReadLine.h"
#include "ezxml.h"
#include "globals.h"
#include "xml_arch.h"

/* special type indexes, necessary for initialization initialization, everything afterwards
   should use the pointers to these type indices*/ 
#define EMPTY_TYPE_INDEX 0
#define IO_TYPE_INDEX 1
    enum Fc_type
{ FC_ABS, FC_FRAC, FC_FULL };


/* Function prototypes */ 
static boolean IsWhitespace(char c);
static void CountTokensInString(IN const char *Str,
				 OUT int *Num,
				 OUT int *Len);
static char **GetNodeTokens(IN ezxml_t Node);
static void CheckElement(IN ezxml_t Node,
			  IN const char *Name);
static void CheckText(IN ezxml_t Node);
static void FreeNode(INOUT ezxml_t Node);
static ezxml_t FindElement(IN ezxml_t Parent,
			    IN const char *Name,
			    IN boolean Required);
static const char *FindProperty(IN ezxml_t Parent,
				 IN const char *Name,
				 IN boolean Required);
static int CountChildren(IN ezxml_t Node,
			  IN const char *Name);
static void ParseFc(ezxml_t Node,
		     enum Fc_type *Fc,
		     float *Val);
static void SetupEmptyType();
static void SetupClassInf(ezxml_t Classes,
			   t_type_descriptor * Type);
static void SetupPinClasses(ezxml_t Classes,
			     t_type_descriptor * Type);
static void SetupPinLocations(ezxml_t Locations,
			       t_type_descriptor * Type);
static void SetupGridLocations(ezxml_t Locations,
				t_type_descriptor * Type);
static void SetupTypeTiming(ezxml_t timing,
			     t_type_descriptor * Type);
static void SetupSubblocksTcomb(ezxml_t timing,
				 t_type_descriptor * Type);
static void SetupSubblocksTSeq(ezxml_t timing_seq_in,
				ezxml_t timing_seq_out,
				t_type_descriptor * Type);
static void Process_Fc(ezxml_t Fc_in_node,
			ezxml_t Fc_out_node,
			t_type_descriptor * Type);
static void ProcessSubblocks(INOUT ezxml_t Parent,
			      INOUT t_type_descriptor * Type,
			      IN boolean timing_enabled);
static void ProcessTypeProps(ezxml_t Node,
			      t_type_descriptor * Type);
static void ProcessChanWidthDistr(INOUT ezxml_t Node,
				   OUT struct s_arch *arch);
static void ProcessChanWidthDistrDir(INOUT ezxml_t Node,
				      OUT t_chan * chan);
static void ProcessLayout(INOUT ezxml_t Node,
			   OUT struct s_arch *arch);
static void ProcessDevice(INOUT ezxml_t Node,
			   OUT struct s_arch *arch,
			   IN boolean timing_enabled);
static void ProcessIO(INOUT ezxml_t Node,
		       IN boolean timing_enabled);
static void ProcessTypes(INOUT ezxml_t Node,
			  OUT t_type_descriptor ** Types,
			  OUT int *NumTypes,
			  IN boolean timing_enabled);
static void ProcessSwitches(INOUT ezxml_t Node,
			     OUT struct s_switch_inf **Switches,
			     OUT int *NumSwitches,
			     IN boolean timing_enabled);
static void ProcessSegments(INOUT ezxml_t Parent,
			     OUT struct s_segment_inf **Segs,
			     OUT int *NumSegs,
			     IN struct s_switch_inf *Switches,
			     IN int NumSwitches,
			     IN boolean timing_enabled);
static void ProcessCB_SB(INOUT ezxml_t Node,
			  INOUT boolean * list,
			  IN int len);


/* Returns TRUE if character is whatspace between tokens */ 
    static boolean
IsWhitespace(char c)
{
    switch (c)
	{
	case ' ':
	case '\t':
	case '\r':
	case '\n':
	    return TRUE;
	default:
	    return FALSE;
	}
}


/* Count tokens and length in all the Text children nodes of current node. */ 
    static void
CountTokensInString(IN const char *Str,
		    OUT int *Num,
		    OUT int *Len)
{
    boolean InToken;
    *Num = 0;
    *Len = 0;
    InToken = FALSE;
    while(*Str)
	{
	    if(IsWhitespace(*Str))
		{
		    InToken = FALSE;
		}
	    
	    else
		
		{
		    if(!InToken)
			{
			    ++(*Num);
			}
		    ++(*Len);
		    InToken = TRUE;
		}
	    ++Str;		/* Advance pointer */
	}
}


/* Returns a token list of the text nodes of a given node. This
 * unlinks all the text nodes from the document */ 
static char **
GetNodeTokens(IN ezxml_t Node)
{
    int Count, Len;
    char *Cur, *Dst;

    boolean InToken;
    char **Tokens;

    
	/* Count the tokens and find length of token data */ 
	CountTokensInString(Node->txt, &Count, &Len);
    
	/* Error out if no tokens found */ 
	if(Count < 1)
	{
	    printf(ERRTAG "Tag '%s' expected enclosed text data " 
		    "but none was found.\n", Node->name);
		exit(1);
	}
    Len = (sizeof(char) * Len) + /* Length of actual data */ 
	(sizeof(char) * Count);	/* Null terminators */
    
	/* Alloc the pointer list and data list. Count the final 
	 * empty string we will use as list terminator */ 
	Tokens = (char **)my_malloc(sizeof(char *) * (Count + 1));
    Dst = (char *)my_malloc(sizeof(char) * Len);
    Count = 0;
    
	/* Copy data to tokens */ 
	Cur = Node->txt;
    InToken = FALSE;
    while(*Cur)
	{
	    if(IsWhitespace(*Cur))
		{
		    if(InToken)
			{
			    *Dst = '\0';
			    ++Dst;
			}
		    InToken = FALSE;
		}
	    
	    else
		{
		    if(!InToken)
			{
			    Tokens[Count] = Dst;
			    ++Count;
			}
		    *Dst = *Cur;
		    ++Dst;
		    InToken = TRUE;
		}
	    ++Cur;
	}
    if(InToken)
	{			/* Null term final token */
	    *Dst = '\0';
	    ++Dst;
	}
    ezxml_set_txt(Node, "");
    Tokens[Count] = NULL;	/* End of tokens marker is a NULL */
    
	/* Return the list */ 
	return Tokens;
}


/* Checks the node is an element with name equal to one given */ 
    static void
CheckElement(IN ezxml_t Node,
	     IN const char *Name)
{
    if(0 != strcmp(Node->name, Name))
	{
	    printf(ERRTAG
		    "Element '%s' within element '%s' does match expected " 
		    "element type of '%s'\n", Node->name, Node->parent->name,
		    Name);
	    exit(1);
	}
}


/* Checks that the node has no remaining child nodes or property nodes, 
 * then unlinks the node from the tree which updates pointers and 
 * frees the memory */ 
    static void
FreeNode(INOUT ezxml_t Node)
{
    ezxml_t Cur;
    char *Txt;

    
	/* Shouldn't have unprocessed properties */ 
	if(Node->attr[0])
	{
	    printf(ERRTAG "Node '%s' has invalid property %s=\"%s\".\n",
		    Node->name, Node->attr[0], Node->attr[1]);
	    exit(1);
	}
    
	/* Shouldn't have non-whitespace text */ 
	Txt = Node->txt;
    while(*Txt)
	{
	    if(!IsWhitespace(*Txt))
		{
		    printf(ERRTAG
			    "Node '%s' has unexpected text '%s' within it.\n",
			    Node->name, Node->txt);
		    exit(1);
		}
	    ++Txt;
	}
    
	/* We shouldn't have child items left */ 
	Cur = Node->child;
    if(Cur)
	{
	    printf(ERRTAG "Node '%s' on has invalid child node '%s'.\n",
		    Node->name, Cur->name);
	    exit(1);
	}
    
	/* Now actually unlink and free the node */ 
	ezxml_remove(Node);
}


/* Finds child element with given name and returns it. Errors out if
 * more than one instance exists. */ 
static ezxml_t
FindElement(IN ezxml_t Parent,
	    IN const char *Name,
	    IN boolean Required)
{
    ezxml_t Cur;
    
	/* Find the first node of correct name */ 
	Cur = ezxml_child(Parent, Name);
    
	/* Error out if node isn't found but they required it */ 
	if(Required)
	{
	    if(NULL == Cur)
		{
		    printf(ERRTAG
			    "Element '%s' not found within element '%s'.\n",
			    Name, Parent->name);
		    exit(1);
		}
	}
    
	/* Look at next tag with same name and error out if exists */ 
	if(Cur != NULL && Cur->next)
	{
	    printf(ERRTAG "Element '%s' found twice within element '%s'.\n",
		    Name, Parent->name);
	    exit(1);
	}
    return Cur;
}
static const char *
FindProperty(IN ezxml_t Parent,
	     IN const char *Name,
	     IN boolean Required)
{
    const char *Res;

    Res = ezxml_attr(Parent, Name);
    if(Required)
	{
	    if(NULL == Res)
		{
		    printf(ERRTAG
			    "Required property '%s' not found for element '%s'.\n",
			    Name, Parent->name);
		    exit(1);
		}
	}
    return Res;
}


/* Counts number of child elements in a container element. 
 * Name is the name of the child element. 
 * Errors if no occurances found. */ 
    static int
CountChildren(IN ezxml_t Node,
	      IN const char *Name)
{
    ezxml_t Cur, sibling;
    int Count;

    Count = 0;
    Cur = Node->child;
    while(Cur)
	{
	    if(strcmp(Cur->name, Name) == 0)
		{
		    ++Count;
		}
	    sibling = Cur->sibling;
	    Cur = Cur->next;
	    if(Cur == NULL)
		{
		    Cur = sibling;
		}
	}
    
	/* Error if no occurances found */ 
	if(Count < 1)
	{
	    printf(ERRTAG "Expected node '%s' to have " 
		    "child elements, but none found.\n", Node->name);
	    exit(1);
	}
    return Count;
}


/* Figures out the Fc type and value for the given node. Unlinks the 
 * type and value. */ 
    static void
ParseFc(ezxml_t Node,
	enum Fc_type *Fc,
	float *Val)
{
    const char *Prop;

    Prop = FindProperty(Node, "type", TRUE);
    if(0 == strcmp(Prop, "abs"))
	{
	    *Fc = FC_ABS;
	}
    
    else if(0 == strcmp(Prop, "frac"))
	{
	    *Fc = FC_FRAC;
	}
    
    else if(0 == strcmp(Prop, "full"))
	{
	    *Fc = FC_FULL;
	}
    
    else
	{
	    printf(ERRTAG "Invalid type '%s' for Fc. Only abs, frac " 
		    "and full are allowed.\n", Prop);
	    exit(1);
	}
    switch (*Fc)
	{
	case FC_FULL:
	    *Val = 0.0;
	    break;
	case FC_ABS:
	case FC_FRAC:
	    *Val = atof(Node->txt);
	    ezxml_set_attr(Node, "type", NULL);
	    ezxml_set_txt(Node, "");
	    break;
	default:
	    assert(0);
	}
    
	/* Release the property */ 
	ezxml_set_attr(Node, "type", NULL);
}


/* Set's up the classinf data of a type and counts the number of 
 * pins the type has. If capacity > 1, we need multiple sets of classes.
 * This correctly sets num_pins to include all the replicated but
 * unique pins that give the capacity. */ 
    static void
SetupClassInf(ezxml_t Classes,
	      t_type_descriptor * Type)
{
    int i, k, CurClass, CurPin, NumClassPins;

    ezxml_t Cur;
    const char *Prop;

    
	/* Alloc class_inf structs */ 
	Type->num_class = Type->capacity * CountChildren(Classes, "class");
    Type->class_inf =
	(t_class *) my_malloc(Type->num_class * sizeof(t_class));
    
	/* Make multiple passes to handle capacity with index increased each pass */ 
	CurPin = 0;
    CurClass = 0;
    Type->num_pins = 0;
    Type->num_drivers = 0;
    Type->num_receivers = 0;
    for(i = 0; i < Type->capacity; ++i)
	{
	    
		/* Restart the parse tree at each node */ 
		Cur = Classes->child;
	    while(Cur)
		{
		    CheckElement(Cur, "class");
		    
			/* Count the number of pins in this class */ 
			CountTokensInString(Cur->txt, &NumClassPins, &k);
		    Type->num_pins += NumClassPins;
		    
			/* Alloc class structures */ 
			Type->class_inf[CurClass].num_pins = NumClassPins;
		    Type->class_inf[CurClass].pinlist =
			(int *)my_malloc(NumClassPins * sizeof(int));
		    for(k = 0; k < NumClassPins; ++k)
			{
			    Type->class_inf[CurClass].pinlist[k] = CurPin;
			    ++CurPin;
			}
		    
			/* Figure out class type */ 
			Prop = FindProperty(Cur, "type", TRUE);
		    if(0 == strcmp(Prop, "in"))
			{
			    Type->num_receivers += NumClassPins;
			    Type->class_inf[CurClass].type = RECEIVER;
			}
		    
		    else if(0 == strcmp(Prop, "out"))
			{
			    Type->num_drivers += NumClassPins;
			    Type->class_inf[CurClass].type = DRIVER;
			}
		    
		    else if(0 == strcmp(Prop, "global"))
			{
			    Type->class_inf[CurClass].type = RECEIVER;
			}
		    
		    else
			{
			    printf(ERRTAG "Invalid pin class type '%s'.\n",
				    Prop);
			    exit(1);
			}
		    ++CurClass;
		    Cur = Cur->next;
		}
	}
}


/* Allocs structures that describe pins and reads pin names. Sets
 * pin/class mappings. Unliinks the class descriptor nodes from XML.
 * For capacity > 1 pins and pinclasses are replicated because while
 * they share the same properties and setup, they are treated as 
 * being unique. */ 
    static void
SetupPinClasses(ezxml_t Classes,
		t_type_descriptor * Type)
{
    int CurClass, i, j, CurPin, PinsPerSubtile, ClassesPerSubtile;

    ezxml_t Cur, Prev;
    const char *Prop;

    boolean IsGlobal;
    char **Tokens;
    int *pin_used = NULL;

    
	/* Allocs and sets up the classinf data and counts pins */ 
	SetupClassInf(Classes, Type);
    PinsPerSubtile = Type->num_pins / Type->capacity;
    ClassesPerSubtile = Type->num_class / Type->capacity;
    
	/* Alloc num_pin sized lists. Replicated pins don't have new names. */ 
	Type->is_global_pin =
	(boolean *) my_malloc(Type->num_pins * sizeof(boolean));
    Type->pin_class = (int *)my_malloc(Type->num_pins * sizeof(int));
    pin_used = (int *)my_malloc(Type->num_pins * sizeof(int));
    for(i = 0; i < Type->num_pins; ++i)
	{
	    pin_used[i] = FALSE;
	}
    
	/* Iterate over all pins */ 
	CurClass = 0;
    Cur = Classes->child;
    while(Cur)
	{
	    CheckElement(Cur, "class");
	    
		/* Figure out if global class */ 
		IsGlobal = FALSE;
	    Prop = FindProperty(Cur, "type", TRUE);
	    if(0 == strcmp(Prop, "global"))
		{
		    IsGlobal = TRUE;
		}
	    ezxml_set_attr(Cur, "type", NULL);
	    
		/* Itterate over pins in the class */ 
		Tokens = GetNodeTokens(Cur);
	    assert(CountTokens(Tokens) ==
		    Type->class_inf[CurClass].num_pins);
	    for(j = 0; j < Type->class_inf[CurClass].num_pins; ++j)
		{
		    CurPin = my_atoi(Tokens[j]);
		    
			/* Check for duplicate pin names */ 
			if(pin_used[CurPin])
			{
			    printf(ERRTAG
				    "Pin %d is defined in two different classes in type '%s'.\n",
				    CurPin, Type->name);
			    exit(1);
			}
		    pin_used[CurPin] = TRUE;
		    
			/* Set the pin class and is_global status */ 
			for(i = 0; i < Type->capacity; ++i)
			{
			    Type->pin_class[CurPin + (i * PinsPerSubtile)] =
				CurClass + (i * ClassesPerSubtile);
			    Type->is_global_pin[CurPin +
						 (i * PinsPerSubtile)] =
				IsGlobal;
			}
		}
	    FreeTokens(&Tokens);
	    ++CurClass;
	    Prev = Cur;
	    Cur = Cur->next;
	    FreeNode(Prev);
	}
    if(pin_used)
	{
	    free(pin_used);
	    pin_used = NULL;
	}
}


/* Sets up the pinloc map for the type. Unlinks the loc nodes
 * from the XML tree.
 * Pins and pin classses must already be setup by SetupPinClasses */ 
    static void
SetupPinLocations(ezxml_t Locations,
		  t_type_descriptor * Type)
{
    int i, j, k, l, PinsPerSubtile, Count, Len;

    ezxml_t Cur, Prev;
    const char *Prop;
    char **Tokens, **CurTokens;

    PinsPerSubtile = Type->num_pins / Type->capacity;
    
	/* Alloc and clear pin locations */ 
	Type->pinloc = (int ***)my_malloc(Type->height * sizeof(int **));
    for(i = 0; i < Type->height; ++i)
	{
	    Type->pinloc[i] = (int **)my_malloc(4 * sizeof(int *));
	    for(j = 0; j < 4; ++j)
		{
		    Type->pinloc[i][j] =
			(int *)my_malloc(Type->num_pins * sizeof(int));
		    for(k = 0; k < Type->num_pins; ++k)
			{
			    Type->pinloc[i][j][k] = 0;
			}
		}
	}
    
	/* Load the pin locations */ 
	Cur = Locations->child;
    while(Cur)
	{
	    CheckElement(Cur, "loc");
	    
		/* Get offset */ 
		i = 0;
	    Prop = FindProperty(Cur, "offset", FALSE);
	    if(Prop)
		{
		    i = my_atoi(Prop);
		    if((i < 0) || (i >= Type->height))
			{
			    printf(ERRTAG
				    "%d is an invalid offset for type '%s'.\n",
				    i, Type->name);
			    exit(1);
			}
		    ezxml_set_attr(Cur, "offset", NULL);
		}
	    
		/* Get side */ 
		Prop = FindProperty(Cur, "side", TRUE);
	    if(0 == strcmp(Prop, "left"))
		{
		    j = LEFT;
		}
	    
	    else if(0 == strcmp(Prop, "top"))
		{
		    j = TOP;
		}
	    
	    else if(0 == strcmp(Prop, "right"))
		{
		    j = RIGHT;
		}
	    
	    else if(0 == strcmp(Prop, "bottom"))
		{
		    j = BOTTOM;
		}
	    
	    else
		{
		    printf(ERRTAG "'%s' is not a valid side.\n", Prop);
		    exit(1);
		}
	    ezxml_set_attr(Cur, "side", NULL);
	    
		/* Check location is on perimeter */ 
		if((TOP == j) && (i != (Type->height - 1)))
		{
		    printf(ERRTAG
			    "Locations are only allowed on large block " 
			    "perimeter. 'top' side should be at offset %d only.\n",
			    (Type->height - 1));
		    exit(1);
		}
	    if((BOTTOM == j) && (i != 0))
		{
		    printf(ERRTAG
			    "Locations are only allowed on large block " 
			    "perimeter. 'bottom' side should be at offset 0 only.\n");
		    exit(1);
		}
	    
		/* Go through lists of pins */ 
		CountTokensInString(Cur->txt, &Count, &Len);
		if(Count > 0) 
		{
			Tokens = GetNodeTokens(Cur);
			CurTokens = Tokens;
			while(*CurTokens)
			{
			    
				/* Get pin */ 
				k = my_atoi(*CurTokens);
				if(k >= Type->num_pins)
				{
					printf(ERRTAG
						"Pin %d of type '%s' is not a valid pin.\n",
						k, Type->name);
					exit(1);
				}
			    
				/* Set pin location */ 
				for(l = 0; l < Type->capacity; ++l)
				{
					Type->pinloc[i][j][k + (l * PinsPerSubtile)] = 1;
				}
			    
				/* Advance through list of pins in this location */ 
				++CurTokens;
			}
			FreeTokens(&Tokens);
		}
	    Prev = Cur;
	    Cur = Cur->next;
	    FreeNode(Prev);
	}
}


/* Sets up the grid_loc_def for the type. Unlinks the loc nodes
 * from the XML tree. */ 
    static void
SetupGridLocations(ezxml_t Locations,
		   t_type_descriptor * Type)
{
    int i;

    ezxml_t Cur, Prev;
    const char *Prop;

    Type->num_grid_loc_def = CountChildren(Locations, "loc");
    Type->grid_loc_def = 
	(struct s_grid_loc_def *)my_calloc(Type->num_grid_loc_def,
					   sizeof(struct s_grid_loc_def));
    
	/* Load the pin locations */ 
	Cur = Locations->child;
    i = 0;
    while(Cur)
	{
	    CheckElement(Cur, "loc");
	    
		/* loc index */ 
		Prop = FindProperty(Cur, "type", TRUE);
	    if(Prop)
		{
		    if(strcmp(Prop, "fill") == 0)
			{
			    if(Type->num_grid_loc_def != 1)
				{
				    printf(ERRTAG
					    "Another loc specified for fill.\n");
				    exit(1);
				}
			    Type->grid_loc_def[i].grid_loc_type = FILL;
			    FILL_TYPE = Type;
			}
		    else if(strcmp(Prop, "col") == 0)
			{
			    Type->grid_loc_def[i].grid_loc_type = COL_REPEAT;
			}
		    else if(strcmp(Prop, "rel") == 0)
			{
			    Type->grid_loc_def[i].grid_loc_type = COL_REL;
			}
		    else
			{
			    printf(ERRTAG
				    "Unknown grid location type '%s' for type '%s'.\n",
				    Prop, Type->name);
			    exit(1);
			}
		    ezxml_set_attr(Cur, "type", NULL);
		}
	    Prop = FindProperty(Cur, "start", FALSE);
	    if(Type->grid_loc_def[i].grid_loc_type == COL_REPEAT)
		{
		    if(Prop == NULL)
			{
			    printf(ERRTAG
				    "grid location property 'start' must be specified for grid location type 'col'.\n");
			    exit(1);
			}
		    Type->grid_loc_def[i].start_col = my_atoi(Prop);
		    ezxml_set_attr(Cur, "start", NULL);
		}
	    else if(Prop != NULL)
		{
		    printf(ERRTAG
			    "grid location property 'start' valid for grid location type 'col' only.\n");
		    exit(1);
		}
	    Prop = FindProperty(Cur, "repeat", FALSE);
	    if(Type->grid_loc_def[i].grid_loc_type == COL_REPEAT)
		{
		    if(Prop != NULL)
			{
			    Type->grid_loc_def[i].repeat = my_atoi(Prop);
			    ezxml_set_attr(Cur, "repeat", NULL);
			}
		}
	    else if(Prop != NULL)
		{
		    printf(ERRTAG
			    "grid location property 'repeat' valid for grid location type 'col' only.\n");
		    exit(1);
		}
	    Prop = FindProperty(Cur, "pos", FALSE);
	    if(Type->grid_loc_def[i].grid_loc_type == COL_REL)
		{
		    if(Prop == NULL)
			{
			    printf(ERRTAG
				    "grid location property 'pos' must be specified for grid location type 'rel'.\n");
			    exit(1);
			}
		    Type->grid_loc_def[i].col_rel = atof(Prop);
		    ezxml_set_attr(Cur, "pos", NULL);
		}
	    else if(Prop != NULL)
		{
		    printf(ERRTAG
			    "grid location property 'pos' valid for grid location type 'rel' only.\n");
		    exit(1);
		}
	    Prop = FindProperty(Cur, "priority", FALSE);
	    if(Prop)
		{
		    Type->grid_loc_def[i].priority = my_atoi(Prop);
		    ezxml_set_attr(Cur, "priority", NULL);
		}
	    Prev = Cur;
	    Cur = Cur->next;
	    FreeNode(Prev);
	    i++;
	}
}
static void
SetupTypeTiming(ezxml_t timing,
		t_type_descriptor * Type)
{
    ezxml_t Cur, Prev;
    const char *Prop;
    float value;
    char **Tokens;

    
	/* Clear timing info for type */ 
	Type->type_timing_inf.T_fb_ipin_to_sblk_ipin = 0;
    Type->type_timing_inf.T_sblk_opin_to_fb_opin = 0;
    Type->type_timing_inf.T_sblk_opin_to_sblk_ipin = 0;
    
	/* Load the timing info */ 
	Cur = timing->child;
    while(Cur)
	{
	    CheckElement(Cur, "tedge");
	    
		/* Get timing type */ 
		Prop = FindProperty(Cur, "type", TRUE);
	    Tokens = GetNodeTokens(Cur);
	    if(CountTokens(Tokens) != 1)
		{
		    printf(ERRTAG
			    "Timing for sequential subblock edges not a number.");
		}
	    value = atof(Tokens[0]);
	    if(0 == strcmp(Prop, "T_fb_ipin_to_sblk_ipin"))
		{
		    Type->type_timing_inf.T_fb_ipin_to_sblk_ipin = value;
		}
	    else if(0 == strcmp(Prop, "T_sblk_opin_to_fb_opin"))
		{
		    Type->type_timing_inf.T_sblk_opin_to_fb_opin = value;
		}
	    else if(0 == strcmp(Prop, "T_sblk_opin_to_sblk_ipin"))
		{
		    Type->type_timing_inf.T_sblk_opin_to_sblk_ipin = value;
		}
	    else
		{
		    printf(ERRTAG "'%s' is an unrecognized timing edge.\n",
			    Prop);
		    exit(1);
		}
	    ezxml_set_attr(Cur, "type", NULL);
	    Prev = Cur;
	    Cur = Cur->next;
	    FreeNode(Prev);
		FreeTokens(&Tokens);
	}
}
static void
SetupSubblocksTcomb(ezxml_t timing,
		    t_type_descriptor * Type)
{
    ezxml_t Cur, prev;
    t_T_subblock ** T_subblock_ptr;
    float **t_comb;
    int i, j;
    char **Tokens;

    T_subblock_ptr = &Type->type_timing_inf.T_subblock;
    t_comb =
	(float **)alloc_matrix(0, Type->max_subblock_inputs - 1, 0,
			       Type->max_subblock_outputs - 1, sizeof(float));
    for(i = 0; i < Type->max_subblock_inputs; i++)
	{
	    for(j = 0; j < Type->max_subblock_outputs; j++)
		{
		    t_comb[i][j] = 0;
		}
	}
    
	/* Load the timing info */ 
	Cur = timing->child;
    i = 0;
    while(Cur)
	{
	    CheckElement(Cur, "trow");
	    Tokens = GetNodeTokens(Cur);
	    if(CountTokens(Tokens) != Type->max_subblock_outputs)
		{
		    printf(ERRTAG
			    "Number of tokens %d not equal number of subblock outputs %d.",
			    CountTokens(Tokens),
			    Type->max_subblock_outputs);
		    exit(1);
		}
	    for(j = 0; j < Type->max_subblock_outputs; j++)
		{
		    t_comb[i][j] = atof(Tokens[j]);
		}
	    i++;
	    prev = Cur;
	    Cur = Cur->next;
	    FreeNode(prev);
		FreeTokens(&Tokens);
	}
    if(i != Type->max_subblock_inputs)
	{
	    printf(ERRTAG
		    "Number of trow %d not equal number of subblock inputs %d.",
		    i, Type->max_subblock_inputs);
	    exit(1);
	}
    for(i = 0; i < Type->max_subblocks; i++)
	{
	    (*T_subblock_ptr)[i].T_comb = t_comb;
	}
}
static void
SetupSubblocksTSeq(ezxml_t timing_seq_in,
		   ezxml_t timing_seq_out,
		   t_type_descriptor * Type)
{
    ezxml_t Cur, prev;
    t_T_subblock ** T_subblock_ptr;
    int i, j;
    float *t_seq[2];

    ezxml_t seq_timing[2];
    char **Tokens;

    T_subblock_ptr = &Type->type_timing_inf.T_subblock;
    seq_timing[0] = timing_seq_in;
    seq_timing[1] = timing_seq_out;
    t_seq[0] = (float *)my_calloc(Type->max_subblock_inputs, sizeof(float));
    t_seq[1] = (float *)my_calloc(Type->max_subblock_outputs, sizeof(float));
    
	/* Load the timing info */ 
	for(i = 0; i < 2; i++)
	{
	    Cur = seq_timing[i]->child;
	    j = 0;
	    while(Cur)
		{
		    CheckElement(Cur, "trow");
		    Tokens = GetNodeTokens(Cur);
		    if(CountTokens(Tokens) != 1)
			{
			    printf(ERRTAG
				    "Timing for sequential subblock edges not a number.");
			}
		    t_seq[i][j] = atof(Tokens[0]);
		    j++;
		    prev = Cur;
		    Cur = Cur->next;
		    FreeNode(prev);
			FreeTokens(&Tokens);
		}
	    if(j != Type->max_subblock_outputs)
		{
		    printf(ERRTAG
			    "Number of trow %d not equal number of subblock outputs %d.",
			    j, Type->max_subblock_outputs);
		    exit(1);
		}
	}
    for(i = 0; i < Type->max_subblocks; i++)
	{
	    (*T_subblock_ptr)[i].T_seq_in = t_seq[0];
	    (*T_subblock_ptr)[i].T_seq_out = t_seq[1];
	}
}


/* Takes in the node ptr for the 'fc_in' and 'fc_out' elements and initializes
 * the appropriate fields of type. Unlinks the contents of the nodes. */ 
    static void
Process_Fc(ezxml_t Fc_in_node,
	   ezxml_t Fc_out_node,
	   t_type_descriptor * Type)
{
    enum Fc_type Type_in;
    enum Fc_type Type_out;

    ParseFc(Fc_in_node, &Type_in, &Type->Fc_in);
    ParseFc(Fc_out_node, &Type_out, &Type->Fc_out);
    if(FC_FULL == Type_in)
	{
	    printf(ERRTAG "'full' Fc type isn't allowed for Fc_in.\n");
	    exit(1);
	}
    Type->is_Fc_out_full_flex = FALSE;
    Type->is_Fc_frac = FALSE;
    if(FC_FULL == Type_out)
	{
	    Type->is_Fc_out_full_flex = TRUE;
	}
    
    else if(Type_in != Type_out)
	{
	    printf(ERRTAG
		    "Fc_in and Fc_out must have same type unless Fc_out has type 'full'.\n");
	    exit(1);
	}
    if(FC_FRAC == Type_in)
	{
	    Type->is_Fc_frac = TRUE;
	}
}


/* This parses contents of the 'subblocks' element and unlinks from tree */ 
    static void
ProcessSubblocks(INOUT ezxml_t Parent,
		 INOUT t_type_descriptor * Type,
		 boolean timing_enabled)
{
    const char *Prop;

    ezxml_t CurType, Cur, Cur2;
    Type->max_subblocks = 1;
    Prop = FindProperty(Parent, "max_subblocks", FALSE);
    if(Prop)
	{
	    Type->max_subblocks = my_atoi(Prop);
	    ezxml_set_attr(Parent, "max_subblocks", NULL);
	}
    Type->max_subblock_inputs = 1;
    Prop = FindProperty(Parent, "max_subblock_inputs", FALSE);
    if(Prop)
	{
	    Type->max_subblock_inputs = my_atoi(Prop);
	    ezxml_set_attr(Parent, "max_subblock_inputs", NULL);
	}
    Type->max_subblock_outputs = 1;
    Prop = FindProperty(Parent, "max_subblock_outputs", FALSE);
    if(Prop)
	{
	    Type->max_subblock_outputs = my_atoi(Prop);
	    ezxml_set_attr(Parent, "max_subblock_outputs", NULL);
	}
    CurType = FindElement(Parent, "timing", timing_enabled);
    if(CurType != NULL)
	{
	    Type->type_timing_inf.T_subblock = 
		(t_T_subblock *) my_malloc(Type->max_subblocks *
					   sizeof(t_T_subblock));
	    
		/* Load T_comb timing for subblock */ 
		Cur = FindElement(CurType, "T_comb", TRUE);
	    SetupSubblocksTcomb(Cur, Type);
	    FreeNode(Cur);
	    
		/* Load Fc */ 
		Cur = FindElement(CurType, "T_seq_in", TRUE);
	    Cur2 = FindElement(CurType, "T_seq_out", TRUE);
	    SetupSubblocksTSeq(Cur, Cur2, Type);
	    FreeNode(Cur);
	    FreeNode(Cur2);
	    FreeNode(CurType);
	}
}


/* Thie processes attributes of the 'type' tag and then unlinks them */ 
    static void
ProcessTypeProps(ezxml_t Node,
		 t_type_descriptor * Type)
{
    const char *Prop;

    
	/* Load type name */ 
	Prop = FindProperty(Node, "name", TRUE);
    Type->name = my_strdup(Prop);
    ezxml_set_attr(Node, "name", NULL);
    
	/* Load capacity */ 
	Type->capacity = 1;	/* DEFAULT */
    
	/* Load height */ 
	Type->height = 1;	/* DEFAULT */
    Prop = FindProperty(Node, "height", FALSE);
    if(Prop)
	{
	    Type->height = my_atoi(Prop);
	    ezxml_set_attr(Node, "height", NULL);
	}
}


/* Takes in node pointing to <layout> and loads all the
 * child type objects. Unlinks the entire <layout> node
 * when complete. */ 
    static void
ProcessLayout(INOUT ezxml_t Node,
	      OUT struct s_arch *arch)
{
    const char *Prop;

    arch->clb_grid.IsAuto = TRUE;
    
	/* Load width and height if applicable */ 
	Prop = FindProperty(Node, "width", FALSE);
    if(Prop != NULL)
	{
	    arch->clb_grid.IsAuto = FALSE;
	    arch->clb_grid.W = my_atoi(Prop);
	    ezxml_set_attr(Node, "width", NULL);
	    Prop = FindProperty(Node, "height", TRUE);
	    if(Prop != NULL)
		{
		    arch->clb_grid.H = my_atoi(Prop);
		    ezxml_set_attr(Node, "height", NULL);
		}
	}
    
	/* Load aspect ratio if applicable */ 
	Prop = FindProperty(Node, "auto", arch->clb_grid.IsAuto);
    if(Prop != NULL)
	{
	    if(arch->clb_grid.IsAuto == FALSE)
		{
		    printf(ERRTAG
			    "Auto-sizing, width and height cannot be specified\n");
		}
	    arch->clb_grid.Aspect = atof(Prop);
	    ezxml_set_attr(Node, "auto", NULL);
	}
}


/* Takes in node pointing to <device> and loads all the
 * child type objects. Unlinks the entire <device> node
 * when complete. */ 
    static void
ProcessDevice(INOUT ezxml_t Node,
	      OUT struct s_arch *arch,
	      IN boolean timing_enabled)
{
    const char *Prop;

    ezxml_t Cur;
    Cur = FindElement(Node, "sizing", TRUE);
    arch->R_minW_nmos = 0;
    Prop = FindProperty(Cur, "R_minW_nmos", timing_enabled);
    if(Prop != NULL)
	{
	    arch->R_minW_nmos = atof(Prop);
	    ezxml_set_attr(Cur, "R_minW_nmos", NULL);
	}
    arch->R_minW_pmos = 0;
    Prop = FindProperty(Cur, "R_minW_pmos", timing_enabled);
    if(Prop != NULL)
	{
	    arch->R_minW_pmos = atof(Prop);
	    ezxml_set_attr(Cur, "R_minW_pmos", NULL);
	}
    arch->ipin_mux_trans_size = 0;
    Prop = FindProperty(Cur, "ipin_mux_trans_size", FALSE);
    if(Prop != NULL)
	{
	    arch->ipin_mux_trans_size = atof(Prop);
	    ezxml_set_attr(Cur, "ipin_mux_trans_size", NULL);
	}
    FreeNode(Cur);
    Cur = FindElement(Node, "timing", timing_enabled);
    if(Cur != NULL)
	{
	    arch->C_ipin_cblock = 0;
	    Prop = FindProperty(Cur, "C_ipin_cblock", FALSE);
	    if(Prop != NULL)
		{
		    arch->C_ipin_cblock = atof(Prop);
		    ezxml_set_attr(Cur, "C_ipin_cblock", NULL);
		}
	    arch->T_ipin_cblock = 0;
	    Prop = FindProperty(Cur, "T_ipin_cblock", FALSE);
	    if(Prop != NULL)
		{
		    arch->T_ipin_cblock = atof(Prop);
		    ezxml_set_attr(Cur, "T_ipin_cblock", NULL);
		}
	    FreeNode(Cur);
	}
    Cur = FindElement(Node, "area", TRUE);
    Prop = FindProperty(Cur, "grid_logic_tile_area", FALSE);
    arch->grid_logic_tile_area = 0;
    if(Prop != NULL)
	{
	    arch->grid_logic_tile_area = atof(Prop);
	    ezxml_set_attr(Cur, "grid_logic_tile_area", NULL);
	}
    FreeNode(Cur);
    Cur = FindElement(Node, "chan_width_distr", FALSE);
    if(Cur != NULL)
	{
	    ProcessChanWidthDistr(Cur, arch);
	    FreeNode(Cur);
	}
    Cur = FindElement(Node, "switch_block", TRUE);
    Prop = FindProperty(Cur, "type", TRUE);
    if(strcmp(Prop, "wilton") == 0)
	{
	    arch->SBType = WILTON;
	}
    else if(strcmp(Prop, "universal") == 0)
	{
	    arch->SBType = UNIVERSAL;
	}
    else if(strcmp(Prop, "subset") == 0)
	{
	    arch->SBType = SUBSET;
	}
    else
	{
	    printf(ERRTAG "Unknown property %s for switch block type x\n",
		    Prop);
	    exit(1);
	}
    ezxml_set_attr(Cur, "type", NULL);
    Prop = FindProperty(Cur, "fs", TRUE);
    arch->Fs = my_atoi(Prop);
    ezxml_set_attr(Cur, "fs", NULL);
    FreeNode(Cur);
}


/* Takes in node pointing to <chan_width_distr> and loads all the
 * child type objects. Unlinks the entire <chan_width_distr> node
 * when complete. */ 
    static void
ProcessChanWidthDistr(INOUT ezxml_t Node,
		      OUT struct s_arch *arch)
{
    const char *Prop;

    ezxml_t Cur;
    Cur = FindElement(Node, "io", TRUE);
    Prop = FindProperty(Cur, "width", TRUE);
    arch->Chans.chan_width_io = atof(Prop);
    ezxml_set_attr(Cur, "width", NULL);
    FreeNode(Cur);
    Cur = FindElement(Node, "x", TRUE);
    ProcessChanWidthDistrDir(Cur, &arch->Chans.chan_x_dist);
    FreeNode(Cur);
    Cur = FindElement(Node, "y", TRUE);
    ProcessChanWidthDistrDir(Cur, &arch->Chans.chan_y_dist);
    FreeNode(Cur);
}


/* Takes in node within <chan_width_distr> and loads all the
 * child type objects. Unlinks the entire node when complete. */ 
    static void
ProcessChanWidthDistrDir(INOUT ezxml_t Node,
			 OUT t_chan * chan)
{
    const char *Prop;

    boolean hasXpeak, hasWidth, hasDc;
    hasXpeak = hasWidth = hasDc = FALSE;
    Prop = FindProperty(Node, "distr", TRUE);
    if(strcmp(Prop, "uniform") == 0)
	{
	    chan->type = UNIFORM;
	}
    else if(strcmp(Prop, "gaussian") == 0)
	{
	    chan->type = GAUSSIAN;
	    hasXpeak = hasWidth = hasDc = TRUE;
	}
    else if(strcmp(Prop, "pulse") == 0)
	{
	    chan->type = PULSE;
	    hasXpeak = hasWidth = hasDc = TRUE;
	}
    else if(strcmp(Prop, "delta") == 0)
	{
	    hasXpeak = hasDc = TRUE;
	    chan->type = DELTA;
	}
    else
	{
	    printf(ERRTAG "Unknown property %s for chan_width_distr x\n",
		    Prop);
	    exit(1);
	}
    ezxml_set_attr(Node, "distr", NULL);
    Prop = FindProperty(Node, "peak", TRUE);
    chan->peak = atof(Prop);
    ezxml_set_attr(Node, "peak", NULL);
    Prop = FindProperty(Node, "width", hasWidth);
    if(hasWidth)
	{
	    chan->width = atof(Prop);
	    ezxml_set_attr(Node, "width", NULL);
	}
    Prop = FindProperty(Node, "xpeak", hasXpeak);
    if(hasXpeak)
	{
	    chan->xpeak = atof(Prop);
	    ezxml_set_attr(Node, "xpeak", NULL);
	}
    Prop = FindProperty(Node, "dc", hasDc);
    if(hasDc)
	{
	    chan->dc = atof(Prop);
	    ezxml_set_attr(Node, "dc", NULL);
	}
}
static void
SetupEmptyType()
{
    t_type_descriptor * type;
    type = &type_descriptors[EMPTY_TYPE->index];
    type->name = "<EMPTY>";
    type->num_pins = 0;
    type->height = 1;
    type->capacity = 0;
    type->num_drivers = 0;
    type->num_receivers = 0;
    type->pinloc = NULL;
    type->num_class = 0;
    type->class_inf = NULL;
    type->pin_class = NULL;
    type->is_global_pin = NULL;
    type->is_Fc_frac = TRUE;
    type->is_Fc_out_full_flex = FALSE;
    type->Fc_in = 0;
    type->Fc_out = 0;
    type->max_subblocks = 0;
    type->max_subblock_inputs = 0;
    type->max_subblock_outputs = 0;
    
	/* Used as lost area filler, no definition */ 
	type->grid_loc_def = NULL;
    type->num_grid_loc_def = 0;
}


/* Takes in node pointing to <io> and loads all the
 * child type objects. Unlinks the entire <io> node
 * when complete. */ 
    static void
ProcessIO(INOUT ezxml_t Node,
	  IN boolean timing_enabled)
{
    const char *Prop;

    ezxml_t Cur, Cur2;
    int i, j;

    t_type_descriptor * type;
    int num_inputs, num_outputs, num_clocks, num_pins, capacity, t_in, t_out;
    enum
    { INCLASS = 0, OUTCLASS = 1, CLKCLASS = 2 };

    type = &type_descriptors[IO_TYPE->index];
    num_inputs = 1;
    num_outputs = 1;
    num_clocks = 1;
    CheckElement(Node, "io");
    type->name = ".io";
    type->height = 1;
    
	/* Load capacity */ 
	Prop = FindProperty(Node, "capacity", TRUE);
    capacity = my_atoi(Prop);
    type->capacity = capacity;
    ezxml_set_attr(Node, "capacity", NULL);
    
	/* Load Fc */ 
	Cur = FindElement(Node, "fc_in", TRUE);
    Cur2 = FindElement(Node, "fc_out", TRUE);
    Process_Fc(Cur, Cur2, type);
    FreeNode(Cur);
    FreeNode(Cur2);
    
	/* Initialize and setup type */ 
	num_pins = 3 * capacity;
    type->num_pins = num_pins;
    type->num_drivers = num_outputs * capacity;
    type->num_receivers = num_inputs * capacity;
    type->pinloc =
	(int ***)alloc_matrix3(0, 0, 0, 3, 0, num_pins - 1, sizeof(int));
    
	/* Jason Luu - September 5, 2007
	 * To treat IOs as any other block in routing, need to blackbox
	 * as having physical pins on all sides.  This is a hack. */ 
	for(i = 0; i < num_pins; ++i)
	{
	    for(j = 0; j < 4; ++j)
		{
		    type->pinloc[0][j][i] = 1;
		}
	}
    
	/* Three fixed classes.  In, Out, Clock */ 
	type->num_class = 3 * capacity;
    type->class_inf = 
	(struct s_class *)my_malloc(sizeof(struct s_class) * 3 * capacity);
    type->pin_class = (int *)my_malloc(sizeof(int) * num_pins);
    type->is_global_pin = (boolean *) my_malloc(sizeof(boolean) * num_pins);
    for(j = 0; j < capacity; ++j)
	{
	    
		/* Three fixed classes. In, Out, Clock */ 
		type->class_inf[3 * j + INCLASS].num_pins = num_inputs;
	    type->class_inf[3 * j + INCLASS].type = RECEIVER;
	    type->class_inf[3 * j + INCLASS].pinlist = 
		(int *)my_malloc(sizeof(int) * num_inputs);
	    for(i = 0; i < num_inputs; ++i)
		{
		    type->class_inf[3 * j + INCLASS].pinlist[i] =
			num_pins * j / capacity + i;
		    type->pin_class[num_pins * j / capacity + i] =
			3 * j + INCLASS;
		    type->is_global_pin[num_pins * j / capacity + i] = FALSE;
		}
	    type->class_inf[3 * j + OUTCLASS].num_pins = num_outputs;
	    type->class_inf[3 * j + OUTCLASS].type = DRIVER;
	    type->class_inf[3 * j + OUTCLASS].pinlist = 
		(int *)my_malloc(sizeof(int) * num_outputs);
	    for(i = 0; i < num_outputs; ++i)
		{
		    type->class_inf[3 * j + OUTCLASS].pinlist[i] =
			num_pins * j / capacity + i + num_inputs;
		    type->pin_class[num_pins * j / capacity + i +
				     num_inputs] = 3 * j + OUTCLASS;
		    type->is_global_pin[num_pins * j / capacity + i +
					 num_inputs] = FALSE;
		}
	    type->class_inf[3 * j + CLKCLASS].num_pins = num_clocks;
	    type->class_inf[3 * j + CLKCLASS].type = RECEIVER;
	    type->class_inf[3 * j + CLKCLASS].pinlist = 
		(int *)my_malloc(sizeof(int) * num_clocks);
	    for(i = 0; i < num_clocks; ++i)
		{
		    type->class_inf[3 * j + CLKCLASS].pinlist[i] =
			num_pins * j / capacity + i + num_inputs +
			num_outputs;
		    type->pin_class[num_pins * j / capacity + i +
				     num_inputs + num_outputs] =
			3 * j + CLKCLASS;
		    type->is_global_pin[num_pins * j / capacity + i +
					 num_inputs + num_outputs] = TRUE;
		}
	}
    type->max_subblocks = 1;
    type->max_subblock_inputs = 1;
    type->max_subblock_outputs = 1;
    
	/* Always at boundary */ 
	type->num_grid_loc_def = 1;
    type->grid_loc_def = 
	(struct s_grid_loc_def *)my_calloc(1, sizeof(struct s_grid_loc_def));
    type->grid_loc_def[0].grid_loc_type = BOUNDARY;
    type->grid_loc_def[0].priority = 0;
    t_in = -1;
    t_out = -1;
    Prop = FindProperty(Node, "t_inpad", timing_enabled);
    if(Prop != NULL)
	{
	    t_in = atof(Prop);
	    ezxml_set_attr(Node, "t_inpad", NULL);
	}
    Prop = FindProperty(Node, "t_outpad", timing_enabled);
    if(Prop != NULL)
	{
	    t_out = atof(Prop);
	    ezxml_set_attr(Node, "t_outpad", NULL);
	}
    if(timing_enabled)
	{
	    type->type_timing_inf.T_fb_ipin_to_sblk_ipin = 0;
	    type->type_timing_inf.T_sblk_opin_to_fb_opin = 0;
	    type->type_timing_inf.T_sblk_opin_to_sblk_ipin = 0;
	    type->type_timing_inf.T_subblock =
		(t_T_subblock *) my_malloc(sizeof(t_T_subblock));
	    type->type_timing_inf.T_subblock[0].T_comb =
		(float **)my_malloc(sizeof(float *));
	    type->type_timing_inf.T_subblock[0].T_comb[0] =
		(float *)my_malloc(sizeof(float));
	    type->type_timing_inf.T_subblock[0].T_comb[0][0] = t_out;
	    type->type_timing_inf.T_subblock[0].T_seq_in =
		(float *)my_malloc(sizeof(float));
	    type->type_timing_inf.T_subblock[0].T_seq_in[0] = 0;
	    type->type_timing_inf.T_subblock[0].T_seq_out =
		(float *)my_malloc(sizeof(float));
	    type->type_timing_inf.T_subblock[0].T_seq_out[0] = t_in;
	}
}


/* Takes in node pointing to <typelist> and loads all the
 * child type objects. Unlinks the entire <typelist> node
 * when complete. */ 
    static void
ProcessTypes(INOUT ezxml_t Node,
	     OUT t_type_descriptor ** Types,
	     OUT int *NumTypes,
	     boolean timing_enabled)
{
    ezxml_t CurType, Prev;
    ezxml_t Cur, Cur2;
    t_type_descriptor * Type;
    int i;

    
	/* Alloc the type list. Need two additional t_type_desctiptors:
	 * 1: empty psuedo-type 
	 * 2: IO type
	 */ 
	*NumTypes = CountChildren(Node, "type") + 2;
    *Types = (t_type_descriptor *) 
	my_malloc(sizeof(t_type_descriptor) * (*NumTypes));
    EMPTY_TYPE = &type_descriptors[EMPTY_TYPE_INDEX];
    IO_TYPE = &type_descriptors[IO_TYPE_INDEX];
    type_descriptors[EMPTY_TYPE_INDEX].index = EMPTY_TYPE_INDEX;
    type_descriptors[IO_TYPE_INDEX].index = IO_TYPE_INDEX;
    SetupEmptyType();
    Cur = FindElement(Node, "io", TRUE);
    ProcessIO(Cur, timing_enabled);
    FreeNode(Cur);
    
	/* Process the types */ 
	i = 2;			/* Skip over 'empty' and 'io' type */
    CurType = Node->child;
    while(CurType)
	{
	    CheckElement(CurType, "type");
	    
		/* Alias to current type */ 
		Type = &(*Types)[i];
	    
		/* Parses the properties fields of the type */ 
		ProcessTypeProps(CurType, Type);
	    
		/* Load subblock info */ 
		Cur = FindElement(CurType, "subblocks", TRUE);
	    ProcessSubblocks(Cur, Type, timing_enabled);
	    FreeNode(Cur);
	    
		/* Load Fc */ 
		Cur = FindElement(CurType, "fc_in", TRUE);
	    Cur2 = FindElement(CurType, "fc_out", TRUE);
	    Process_Fc(Cur, Cur2, Type);
	    FreeNode(Cur);
	    FreeNode(Cur2);
	    
		/* Load pin names and classes and locations */ 
		Cur = FindElement(CurType, "pinclasses", TRUE);
	    SetupPinClasses(Cur, Type);
	    FreeNode(Cur);
	    Cur = FindElement(CurType, "pinlocations", TRUE);
	    SetupPinLocations(Cur, Type);
	    FreeNode(Cur);
	    Cur = FindElement(CurType, "gridlocations", TRUE);
	    SetupGridLocations(Cur, Type);
	    FreeNode(Cur);
	    Cur = FindElement(CurType, "timing", timing_enabled);
	    if(Cur)
		{
		    SetupTypeTiming(Cur, Type);
		    FreeNode(Cur);
		}
	    Type->index = i;
	    
		/* Type fully read */ 
		++i;
	    
		/* Free this node and get its next sibling node */ 
		Prev = CurType;
	    CurType = CurType->next;
	    FreeNode(Prev);
	}
    if(FILL_TYPE == NULL)
	{
	    printf(ERRTAG "grid location type 'fill' must be specified.\n");
	    exit(1);
	}
}


/* Loads the given architecture file. Currently only 
 * handles type information */ 
    void
XmlReadArch(IN const char *ArchFile,
	    IN boolean timing_enabled,
	    OUT struct s_arch *arch,
	    OUT t_type_descriptor ** Types,
	    OUT int *NumTypes)
{
    ezxml_t Cur, Next;
    
	/* Parse the file */ 
	Cur = ezxml_parse_file(ArchFile);
    if(NULL == Cur)
	{
	    printf(ERRTAG "Unable to load architecture file '%s'.\n",
		    ArchFile);
	}
    
	/* Root node should be architecture */ 
	CheckElement(Cur, "architecture");
    
	/* Process layout */ 
	Next = FindElement(Cur, "layout", TRUE);
    ProcessLayout(Next, arch);
    FreeNode(Next);
    
	/* Process device */ 
	Next = FindElement(Cur, "device", TRUE);
    ProcessDevice(Next, arch, timing_enabled);
    FreeNode(Next);
    
	/* Process types */ 
	Next = FindElement(Cur, "typelist", TRUE);
    ProcessTypes(Next, Types, NumTypes, timing_enabled);
    FreeNode(Next);
    
	/* Process switches */ 
	Next = FindElement(Cur, "switchlist", TRUE);
    ProcessSwitches(Next, &(arch->Switches), &(arch->num_switches),
		     timing_enabled);
    FreeNode(Next);
    
	/* Process segments. This depends on switches */ 
	Next = FindElement(Cur, "segmentlist", TRUE);
    ProcessSegments(Next, &(arch->Segments), &(arch->num_segments),
		     arch->Switches, arch->num_switches, timing_enabled);
    FreeNode(Next);
    
	/* Release the full XML tree */ 
	FreeNode(Cur);
}
static void
ProcessSegments(INOUT ezxml_t Parent,
		OUT struct s_segment_inf **Segs,
		OUT int *NumSegs,
		IN struct s_switch_inf *Switches,
		IN int NumSwitches,
		IN boolean timing_enabled)
{
    int i, j, length;
    const char *tmp;

    ezxml_t SubElem;
    ezxml_t Node;
    
	/* Count the number of segs and check they are in fact
	 * of segment elements. */ 
	*NumSegs = CountChildren(Parent, "segment");
    
	/* Alloc segment list */ 
	*Segs = NULL;
    if(*NumSegs > 0)
	{
	    *Segs =
		(struct s_segment_inf *)my_malloc(*NumSegs *
						   sizeof(struct
							  s_segment_inf));
	    memset(*Segs, 0, (*NumSegs * sizeof(struct s_segment_inf)));
	}
    
	/* Load the segments. */ 
	for(i = 0; i < *NumSegs; ++i)
	{
	    Node = ezxml_child(Parent, "segment");
	    
		/* Get segment length */ 
		length = 1;	/* DEFAULT */
	    tmp = FindProperty(Node, "length", FALSE);
	    if(tmp)
		{
		    if(strcmp(tmp, "longline") == 0)
			{
			    (*Segs)[i].longline = TRUE;
			}
		    else
			{
			    length = my_atoi(tmp);
			}
		}
	    (*Segs)[i].length = length;
	    ezxml_set_attr(Node, "length", NULL);
	    
		/* Get the frequency */ 
		(*Segs)[i].frequency = 1;	/* DEFAULT */
	    tmp = FindProperty(Node, "freq", FALSE);
	    if(tmp)
		{
		    (*Segs)[i].frequency = (int) (atof(tmp) * MAX_CHANNEL_WIDTH);
		}
	    ezxml_set_attr(Node, "freq", NULL);
	    
		/* Get timing info */ 
		(*Segs)[i].Rmetal = 0;	/* DEFAULT */
	    tmp = FindProperty(Node, "Rmetal", timing_enabled);
	    if(tmp)
		{
		    (*Segs)[i].Rmetal = atof(tmp);
		}
	    ezxml_set_attr(Node, "Rmetal", NULL);
	    (*Segs)[i].Cmetal = 0;	/* DEFAULT */
	    tmp = FindProperty(Node, "Cmetal", timing_enabled);
	    if(tmp)
		{
		    (*Segs)[i].Cmetal = atof(tmp);
		}
	    ezxml_set_attr(Node, "Cmetal", NULL);
	    
		/* Get the type */ 
		tmp = FindProperty(Node, "type", TRUE);
	    if(0 == strcmp(tmp, "bidir"))
		{
		    (*Segs)[i].directionality = BI_DIRECTIONAL;
		}
	    
	    else if(0 == strcmp(tmp, "unidir"))
		{
		    (*Segs)[i].directionality = UNI_DIRECTIONAL;
		}
	    
	    else
		{
		    printf(ERRTAG "Invalid switch type '%s'.\n", tmp);
		    exit(1);
		}
	    ezxml_set_attr(Node, "type", NULL);
	    
		/* Get the wire and opin switches, or mux switch if unidir */ 
		if(UNI_DIRECTIONAL == (*Segs)[i].directionality)
		{
		    SubElem = FindElement(Node, "mux", TRUE);
		    tmp = FindProperty(SubElem, "name", TRUE);
		    
			/* Match names */ 
			for(j = 0; j < NumSwitches; ++j)
			{
			    if(0 == strcmp(tmp, Switches[j].name))
				{
				    break;	/* End loop so j is where we want it */
				}
			}
		    if(j >= NumSwitches)
			{
			    printf(ERRTAG "'%s' is not a valid mux name.\n",
				    tmp);
			    exit(1);
			}
		    ezxml_set_attr(SubElem, "name", NULL);
		    FreeNode(SubElem);
		    
			/* Unidir muxes must have the same switch
			 * for wire and opin fanin since there is 
			 * really only the mux in unidir. */ 
			(*Segs)[i].wire_switch = j;
		    (*Segs)[i].opin_switch = j;
		}
	    
	    else
		{
		    assert(BI_DIRECTIONAL == (*Segs)[i].directionality);
		    SubElem = FindElement(Node, "wire_switch", TRUE);
		    tmp = FindProperty(SubElem, "name", TRUE);
		    
			/* Match names */ 
			for(j = 0; j < NumSwitches; ++j)
			{
			    if(0 == strcmp(tmp, Switches[j].name))
				{
				    break;	/* End loop so j is where we want it */
				}
			}
		    if(j >= NumSwitches)
			{
			    printf(ERRTAG
				    "'%s' is not a valid wire_switch name.\n",
				    tmp);
			    exit(1);
			}
		    (*Segs)[i].wire_switch = j;
		    ezxml_set_attr(SubElem, "name", NULL);
		    FreeNode(SubElem);
		    SubElem = FindElement(Node, "opin_switch", TRUE);
		    tmp = FindProperty(SubElem, "name", TRUE);
		    
			/* Match names */ 
			for(j = 0; j < NumSwitches; ++j)
			{
			    if(0 == strcmp(tmp, Switches[j].name))
				{
				    break;	/* End loop so j is where we want it */
				}
			}
		    if(j >= NumSwitches)
			{
			    printf(ERRTAG
				    "'%s' is not a valid opin_switch name.\n",
				    tmp);
			    exit(1);
			}
		    (*Segs)[i].opin_switch = j;
		    ezxml_set_attr(SubElem, "name", NULL);
		    FreeNode(SubElem);
		}
	    
		/* Setup the CB list if they give one, otherwise use full */ 
		(*Segs)[i].cb_len = length;
	    (*Segs)[i].cb = (boolean *) my_malloc(length * sizeof(boolean));
	    for(j = 0; j < length; ++j)
		{
		    (*Segs)[i].cb[j] = TRUE;
		}
	    SubElem = FindElement(Node, "cb", FALSE);
	    if(SubElem)
		{
		    ProcessCB_SB(SubElem, (*Segs)[i].cb, length);
		    FreeNode(SubElem);
		}
	    
		/* Setup the SB list if they give one, otherwise use full */ 
		(*Segs)[i].sb_len = (length + 1);
	    (*Segs)[i].sb =
		(boolean *) my_malloc((length + 1) * sizeof(boolean));
	    for(j = 0; j < (length + 1); ++j)
		{
		    (*Segs)[i].sb[j] = TRUE;
		}
	    SubElem = FindElement(Node, "sb", FALSE);
	    if(SubElem)
		{
		    ProcessCB_SB(SubElem, (*Segs)[i].sb, (length + 1));
		    FreeNode(SubElem);
		}
	    FreeNode(Node);
	}
}
static void
ProcessCB_SB(INOUT ezxml_t Node,
	     INOUT boolean * list,
	     IN int len)
{
    const char *tmp = NULL;
    int i;

    
	/* Check the type. We only support 'pattern' for now. 
	 * Should add frac back eventually. */ 
	tmp = FindProperty(Node, "type", TRUE);
    if(0 == strcmp(tmp, "pattern"))
	{
	    i = 0;
	    
		/* Get the content string */ 
		tmp = Node->txt;
	    while(*tmp)
		{
		    switch (*tmp)
			{
			case ' ':
			    break;
			case 'T':
			case '1':
			    if(i >= len)
				{
				    printf(ERRTAG
					    "CB or SB depopulation is too long. It "
					    
					    "should be (length) symbols for CBs and (length+1) "
					     "symbols for SBs.\n");
				    exit(1);
				}
			    list[i] = TRUE;
			    ++i;
			    break;
			case 'F':
			case '0':
			    if(i >= len)
				{
				    printf(ERRTAG
					    "CB or SB depopulation is too long. It "
					    
					    "should be (length) symbols for CBs and (length+1) "
					     "symbols for SBs.\n");
				    exit(1);
				}
			    list[i] = FALSE;
			    ++i;
			    break;
			default:
			    printf(ERRTAG "Invalid character %c in CB or " 
				    "SB depopulation list.\n", *tmp);
			    exit(1);
			}
		    ++tmp;
		}
	    if(i < len)
		{
		    printf(ERRTAG "CB or SB depopulation is too short. It " 
			    "should be (length) symbols for CBs and (length+1) "
			     "symbols for SBs.\n");
		    exit(1);
		}
	    
		/* Free content string */ 
		ezxml_set_txt(Node, "");
	}
    
    else
	{
	    printf(ERRTAG "'%s' is not a valid type for specifying " 
		    "cb and sb depopulation.\n", tmp);
	    exit(1);
	}
    ezxml_set_attr(Node, "type", NULL);
}
static void
ProcessSwitches(INOUT ezxml_t Parent,
		OUT struct s_switch_inf **Switches,
		OUT int *NumSwitches,
		IN boolean timing_enabled)
{
    int i, j;
    const char *type_name;
    const char *switch_name;
    const char *Prop;

    boolean has_buf_size;
    ezxml_t Node;
    has_buf_size = FALSE;
    
	/* Count the children and check they are switches */ 
	*NumSwitches = CountChildren(Parent, "switch");
    
	/* Alloc switch list */ 
	*Switches = NULL;
    if(*NumSwitches > 0)
	{
	    *Switches =
		(struct s_switch_inf *)my_malloc(*NumSwitches *
						  sizeof(struct
							 s_switch_inf));
	    memset(*Switches, 0,
		    (*NumSwitches * sizeof(struct s_switch_inf)));
	}
    
	/* Load the switches. */ 
	for(i = 0; i < *NumSwitches; ++i)
	{
	    Node = ezxml_child(Parent, "switch");
	    switch_name = FindProperty(Node, "name", TRUE);
	    type_name = FindProperty(Node, "type", TRUE);
	    
		/* Check for switch name collisions */ 
		for(j = 0; j < i; ++j)
		{
		    if(0 == strcmp((*Switches)[j].name, switch_name))
			{
			    printf(ERRTAG
				    "Two switches with the same name '%s' were "
				     "found.\n", switch_name);
			    exit(1);
			}
		}
	    (*Switches)[i].name = my_strdup(switch_name);
	    ezxml_set_attr(Node, "name", NULL);
	    
		/* Figure out the type of switch. */ 
		if(0 == strcmp(type_name, "mux"))
		{
		    (*Switches)[i].buffered = TRUE;
		    has_buf_size = TRUE;
		}
	    
	    else if(0 == strcmp(type_name, "pass_trans"))
		{
		    (*Switches)[i].buffered = FALSE;
		}
	    
	    else if(0 == strcmp(type_name, "buffer"))
		{
		    (*Switches)[i].buffered = TRUE;
		}
	    
	    else
		{
		    printf(ERRTAG "Invalid switch type '%s'.\n", type_name);
		    exit(1);
		}
	    ezxml_set_attr(Node, "type", NULL);
	    Prop = FindProperty(Node, "R", timing_enabled);
	    if(Prop != NULL)
		{
		    (*Switches)[i].R = atof(Prop);
		    ezxml_set_attr(Node, "R", NULL);
		}
	    Prop = FindProperty(Node, "Cin", timing_enabled);
	    if(Prop != NULL)
		{
		    (*Switches)[i].Cin = atof(Prop);
		    ezxml_set_attr(Node, "Cin", NULL);
		}
	    Prop = FindProperty(Node, "Cout", timing_enabled);
	    if(Prop != NULL)
		{
		    (*Switches)[i].Cout = atof(Prop);
		    ezxml_set_attr(Node, "Cout", NULL);
		}
	    Prop = FindProperty(Node, "Tdel", timing_enabled);
	    if(Prop != NULL)
		{
		    (*Switches)[i].Tdel = atof(Prop);
		    ezxml_set_attr(Node, "Tdel", NULL);
		}
	    Prop = FindProperty(Node, "buf_size", has_buf_size);
	    if(has_buf_size)
		{
		    (*Switches)[i].buf_size = atof(Prop);
		    ezxml_set_attr(Node, "buf_size", NULL);
		}
	    Prop = FindProperty(Node, "mux_trans_size", FALSE);
	    if(Prop != NULL)
		{
		    (*Switches)[i].mux_trans_size = atof(Prop);
		    ezxml_set_attr(Node, "mux_trans_size", NULL);
		}
	    
		/* Remove the switch element from parse tree */ 
		FreeNode(Node);
	}
}


/* Output the data from architecture data so user can verify it
 * was interpretted correctly. */ 
    void
EchoArch(IN const char *EchoFile,
	 IN const t_type_descriptor * Types,
	 IN int NumTypes)
{
    int i, j, k;

    FILE * Echo;
    Echo = my_fopen(EchoFile, "w");
    for(i = 0; i < NumTypes; ++i)
	{
	    fprintf(Echo, "Type: \"%s\"\n", Types[i].name);
	    fprintf(Echo, "\tcapacity: %d\n", Types[i].capacity);
	    fprintf(Echo, "\theight: %d\n", Types[i].height);
	    if(Types[i].num_pins > 0)
		{
		    for(j = 0; j < Types[i].height; ++j)
			{
			    fprintf(Echo,
				     "\tpinloc[%d] TOP LEFT BOTTOM RIGHT:\n",
				     j);
			    for(k = 0; k < Types[i].num_pins; ++k)
				{
				    fprintf(Echo, "\t\t%d %d %d %d\n",
					     Types[i].pinloc[j][TOP][k],
					     Types[i].pinloc[j][LEFT][k],
					     Types[i].pinloc[j][BOTTOM][k],
					     Types[i].pinloc[j][RIGHT][k]);
				}
			}
		}
	    fprintf(Echo, "\tnum_pins (scaled for capacity): %d\n",
		      Types[i].num_pins);
	    if(Types[i].num_pins > 0)
		{
		    fprintf(Echo, "\tPins: NAME CLASS IS_GLOBAL\n");
		    for(j = 0; j < Types[i].num_pins; ++j)
			{
			    fprintf(Echo, "\t\t%d %d %s\n", j,
				     Types[i].pin_class[j],
				     (Types[i].
				       is_global_pin[j] ? "TRUE" : "FALSE"));
			}
		}
	    fprintf(Echo, "\tnum_class: %d\n", Types[i].num_class);
	    if(Types[i].num_class > 0)
		{
		    for(j = 0; j < Types[i].num_class; ++j)
			{
			    switch (Types[i].class_inf[j].type)
				{
				case RECEIVER:
				    fprintf(Echo, "\t\tType: RECEIVER\n");
				    break;
				case DRIVER:
				    fprintf(Echo, "\t\tType: DRIVER\n");
				    break;
				case OPEN:
				    fprintf(Echo, "\t\tType: OPEN\n");
				    break;
				default:
				    fprintf(Echo, "\t\tType: UNKNOWN\n");
				    break;
				}
			    fprintf(Echo, "\t\t\tnum_pins: %d\n",
				     Types[i].class_inf[j].num_pins);
			    fprintf(Echo, "\t\t\tpins: ");	/* No \n */
			    for(k = 0; k < Types[i].class_inf[j].num_pins;
				 ++k)
				{
				    fprintf(Echo, "%d ", Types[i].class_inf[j].pinlist[k]);	/* No \n */
				}
			    fprintf(Echo, "\n");	/* End current line */
			}
		}
	    fprintf(Echo, "\tis_Fc_frac: %s\n",
		      (Types[i].is_Fc_frac ? "TRUE" : "FALSE"));
	    fprintf(Echo, "\tis_Fc_out_full_flex: %s\n",
		     (Types[i].is_Fc_out_full_flex ? "TRUE" : "FALSE"));
	    fprintf(Echo, "\tFc_in: %f\n", Types[i].Fc_in);
	    fprintf(Echo, "\tFc_out: %f\n", Types[i].Fc_out);
	    fprintf(Echo, "\tmax_subblocks: %d\n", Types[i].max_subblocks);
	    fprintf(Echo, "\tmax_subblock_inputs: %d\n",
		     Types[i].max_subblock_inputs);
	    fprintf(Echo, "\tmax_subblock_outputs: %d\n",
		     Types[i].max_subblock_outputs);
	    fprintf(Echo, "\tnum_drivers: %d\n", Types[i].num_drivers);
	    fprintf(Echo, "\tnum_receivers: %d\n", Types[i].num_receivers);
	    fprintf(Echo, "\tindex: %d\n", Types[i].index);
	    fprintf(Echo, "\n");
	}
    fclose(Echo);
}


