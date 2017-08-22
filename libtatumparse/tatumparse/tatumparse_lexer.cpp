#include "tatumparse_lexer.hpp"
#include "tatumparse_lexer.gen.h" //For tatumparse_lex_*()

extern YY_DECL; //For tatumparse_lex()

namespace tatumparse {

Lexer::Lexer(FILE* file, Callback& callback)
    : callback_(callback) {
    tatumparse_lex_init(&state_);
    tatumparse_set_in(file, state_);
}

Lexer::~Lexer() {
    tatumparse_lex_destroy(state_);
}

Parser::symbol_type Lexer::next_token() {
    return tatumparse_lex(state_, callback_);
}

const char* Lexer::text() const {
    return tatumparse_get_text(state_);
}

int Lexer::lineno() const {
    return tatumparse_get_lineno(state_);
}

}
