#include "blif_lexer.hpp"
#include "blif_lexer.gen.h" //For blifparse_lex_*()

extern YY_DECL; //For blifparse_lex()

namespace blifparse {

Lexer::Lexer(FILE* file) {
    blifparse_lex_init(&state_);
    blifparse_set_in(file, state_);
}

Lexer::~Lexer() {
    blifparse_lex_destroy(state_);
}

Parser::symbol_type Lexer::next_token() {
    return blifparse_lex(state_);
}

const char* Lexer::text() const {
    return blifparse_get_text(state_);
}

int Lexer::lineno() const {
    return blifparse_get_lineno(state_);
}

}
