#ifndef EXPR_EVAL_H
#define EXPR_EVAL_H

/**** Structs ****/
/* contains data passed in to the switchblock parser */
struct t_formula_data{
	int dest_W;	/* number of potential wire segments we can connect to in the destination channel */
	int wire;	/* incoming wire index */
};

/* returns integer result according to specified formula and data */
int parse_formula( const char *formula, const t_formula_data &mydata);

/* returns integer result according to specified piece-wise formula and data */
int parse_piecewise_formula( const char *formula, const t_formula_data &mydata);

/* checks if the specified formula is piece-wise defined */
bool is_piecewise_formula( const char *formula);
#endif
