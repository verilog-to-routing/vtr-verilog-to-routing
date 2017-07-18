#include <cstdio>

#include "tatumparse.hpp"

#include "tatumparse_common.hpp"
#include "tatumparse_lexer.hpp"
#include "tatumparse_error.hpp"


namespace tatumparse {

/*
 * Given a filename parses the file as an BLIF file
 * and returns a pointer to a struct containing all
 * the tatum commands.  See tatum.h for data structure
 * detials.
 */
void tatum_parse_filename(std::string filename, Callback& callback) {
    tatum_parse_filename(filename.c_str(), callback);
}

void tatum_parse_filename(const char* filename, Callback& callback) {
    FILE* infile = std::fopen(filename, "r");
    if(infile != NULL) {
        //Parse the file
        tatum_parse_file(infile, callback, filename);

        std::fclose(infile);
    } else {
        tatum_error_wrap(callback, 0, "", "Could not open file '%s'.\n", filename);
    }
}

void tatum_parse_file(FILE* tatum_file, Callback& callback, const char* filename) {

    //Initialize the lexer
    Lexer lexer(tatum_file, callback);

    //Setup the parser + lexer
    Parser parser(lexer, callback);

    //Just before parsing starts
    callback.start_parse();

    //Tell the caller the file name
    callback.filename(filename); 

    //Do the actual parse
    int error = parser.parse();
    if(error) {
        tatum_error_wrap(callback, 0, "", "File failed to parse.\n");
    }

    //Finished parsing
    callback.finish_parse();
}

}
