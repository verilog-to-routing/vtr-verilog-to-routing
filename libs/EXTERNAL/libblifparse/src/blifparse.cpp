#include <cstdio>

#include "blifparse.hpp"

#include "blif_common.hpp"
#include "blif_lexer.hpp"
#include "blif_error.hpp"


namespace blifparse {

//.conn [Extended BLIF]
void Callback::conn(std::string /*src*/, std::string /*dst*/) {
    parse_error(-1, ".conn", "Unsupported BLIF extension");
}

//.cname [Extended BLIF]
void Callback::cname(std::string /*cell_name*/) {
    parse_error(-1, ".cname", "Unsupported BLIF extension");
}

//.attr [Extended BLIF]
void Callback::attr(std::string /*name*/, std::string /*value*/) {
    parse_error(-1, ".attr", "Unsupported BLIF extension");
}

//.param [Extended BLIF]
void Callback::param(std::string /*name*/, std::string /*value*/) {
    parse_error(-1, ".param", "Unsupported BLIF extension");
}

/*
 * Given a filename parses the file as an BLIF file
 * and returns a pointer to a struct containing all
 * the blif commands.  See blif.h for data structure
 * detials.
 */
void blif_parse_filename(std::string filename, Callback& callback) {
    blif_parse_filename(filename.c_str(), callback);
}

void blif_parse_filename(const char* filename, Callback& callback) {
    FILE* infile = std::fopen(filename, "r");
    if(infile != NULL) {
        //Parse the file
        blif_parse_file(infile, callback, filename);

        std::fclose(infile);
    } else {
        blif_error_wrap(callback, 0, "", "Could not open file '%s'.\n", filename);
    }
}

void blif_parse_file(FILE* blif_file, Callback& callback, const char* filename) {

    //Initialize the lexer
    Lexer lexer(blif_file, callback);

    //Setup the parser + lexer
    Parser parser(lexer, callback);

    //Just before parsing starts
    callback.start_parse();

    //Tell the caller the file name
    callback.filename(filename); 

    //Do the actual parse
    int error = parser.parse();
    if(error) {
        blif_error_wrap(callback, 0, "", "File failed to parse.\n");
    }

    //Finished parsing
    callback.finish_parse();
}

}
