/**
 * Jason Luu
 * July 22, 2009
 * Tokenizer
 */

#ifndef TOKEN_H
#define TOKEN_H

enum e_token_type {
	TOKEN_NULL,
	TOKEN_STRING,
	TOKEN_INT,
	TOKEN_OPEN_SQUARE_BRACKET,
	TOKEN_CLOSE_SQUARE_BRACKET,
	TOKEN_OPEN_SQUIG_BRACKET,
	TOKEN_CLOSE_SQUIG_BRACKET,
	TOKEN_COLON,
	TOKEN_DOT
};

struct s_token {
	enum e_token_type type;
	char *data;
};
typedef struct s_token t_token;

t_token *GetTokensFromString(INP const char* inString, OUTP int * num_tokens);

void freeTokens(INP t_token *tokens, INP int num_tokens);

boolean checkTokenType(INP t_token token, OUTP enum e_token_type token_type);

void my_atof_2D(INOUTP float **matrix, INP int max_i, INP int max_j, INP char *instring);

bool check_my_atof_2D(INP int max_i, INP int max_j,
		INP char *instring, OUTP int* num_entries);

#endif

