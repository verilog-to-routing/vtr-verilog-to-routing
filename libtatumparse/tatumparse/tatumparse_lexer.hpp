#ifndef TATUM_LEXER_HPP
#define TATUM_LEXER_HPP

#include "tatumparse.hpp" //For tatumparse::Callback
#include "tatumparse_parser.hpp" //For Parser::symbol_type

namespace tatumparse {

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

} //namespace

/*
 * The YY_DECL is used by flex to specify the signature of the main
 * lexer function.
 *
 * We re-define it to something reasonable
 */
#undef YY_DECL
#define YY_DECL tatumparse::Parser::symbol_type tatumparse_lex(yyscan_t yyscanner, tatumparse::Callback& callback)

#endif
