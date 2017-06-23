#include "sdc_lexer.hpp"

//Windows doesn't have unistd.h, so we set '%option nounistd' 
//in sdc_lexer.l, but flex still includes it in the generated 
//header unless YY_NO_UNISTD_H is defined to 1
#define YY_NO_UNISTD_H 1
#include "sdc_lexer.gen.hpp" //For sdcparse_lex_*()

extern YY_DECL; //For sdcparse_lex()

namespace sdcparse {

Lexer::Lexer(FILE* file, Callback& callback)
    : callback_(callback) {
    sdcparse_lex_init(&state_);
    sdcparse_set_in(file, state_);
}

Lexer::~Lexer() {
    sdcparse_lex_destroy(state_);
}

Parser::symbol_type Lexer::next_token() {
    return sdcparse_lex(state_, callback_);
}

const char* Lexer::text() const {
    return sdcparse_get_text(state_);
}

int Lexer::lineno() const {
    return sdcparse_get_lineno(state_);
}

}
