#include "sdc_lexer.hpp"
#include "sdc_lexer.gen.h" //For sdcparse_lex_*()

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
