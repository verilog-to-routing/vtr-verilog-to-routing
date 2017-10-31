#include "blif_lexer.hpp"

//Windows doesn't have unistd.h, so we set '%option nounistd' 
//in blif_lexer.l, but flex still includes it in the generated 
//header unless YY_NO_UNISTD_H is defined to 1
#define YY_NO_UNISTD_H 1
#include "blif_lexer.gen.hpp" //For blifparse_lex_*()

extern YY_DECL; //For blifparse_lex()

namespace blifparse {

Lexer::Lexer(FILE* file, Callback& callback)
    : callback_(callback) {
    blifparse_lex_init(&state_);
    blifparse_set_in(file, state_);
}

Lexer::~Lexer() {
    blifparse_lex_destroy(state_);
}

Parser::symbol_type Lexer::next_token() {
    return blifparse_lex(state_, callback_);
}

const char* Lexer::text() const {
    return blifparse_get_text(state_);
}

int Lexer::lineno() const {
    return blifparse_get_lineno(state_);
}

}
