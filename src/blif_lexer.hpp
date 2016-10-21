#ifndef BLIF_LEXER_HPP
#define BLIF_LEXER_HPP

#include "blifparse.hpp" //For blifparse::Callback
#include "blif_parser.hpp" //For Parser::symbol_type

namespace blifparse {

typedef void* yyscan_t;

class Lexer {
    public:
        Lexer(FILE* file, Callback& callback);
        ~Lexer();
        Parser::symbol_type next_token();
        const char* text() const;
        int lineno() const;
    private:
        yyscan_t state_;
        Callback& callback_;
};

/*
 * The YY_DECL is used by flex to specify the signature of the main
 * lexer function.
 *
 * We re-define it to something reasonable
 */
#undef YY_DECL
#define YY_DECL blifparse::Parser::symbol_type blifparse_lex(yyscan_t yyscanner, blifparse::Callback& callback)

} //namespace
#endif
