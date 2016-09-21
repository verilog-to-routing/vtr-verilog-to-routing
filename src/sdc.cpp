#include <stdio.h>

#include "sdc.hpp"
#include "sdc_common.hpp"

#include "sdc_lexer.hpp"
#include "sdc_error.hpp"


extern FILE	*sdcparse_in; //Global input file defined by flex

namespace sdcparse {

/*
 * Given a filename parses the file as an SDC file
 * and returns a pointer to a struct containing all
 * the sdc commands.  See sdc.h for data structure
 * detials.
 */
std::shared_ptr<SdcCommands> sdc_parse_filename(std::string filename) {
    return sdc_parse_filename(filename.c_str());
}

std::shared_ptr<SdcCommands> sdc_parse_filename(const char* filename) {
    std::shared_ptr<SdcCommands> sdc_commands;

    FILE* infile = fopen(filename, "r");
    if(infile != NULL) {
        sdc_commands = sdc_parse_file(infile);
        fclose(infile);
    } else {
        fclose(infile);
        sdc_error_wrap(0, "", "Could not open file %s.\n", filename);
    }

    return sdc_commands;
}

std::shared_ptr<SdcCommands> sdc_parse_file(FILE* sdc_file) {

    //Initialize the lexer
    Lexer lexer(sdc_file);

    auto sdc_commands = std::make_shared<SdcCommands>();

    //Setup the pares + lexer
    Parser parser(lexer, sdc_commands);

    //Do the parse
    int error = parser.parse();
    if(error) {
        sdc_error_wrap(0, "", "SDC Error: file failed to parse!\n");
    }

    return sdc_commands;
}

/*
 * Error handling
 */
void default_sdc_error(const int line_no, const std::string& near_text, const std::string& msg) {
    fprintf(stderr, "SDC Error line %d near '%s': %s\n", line_no, near_text.c_str(), msg.c_str());
    exit(1);
}

void set_sdc_error_handler(std::function<void(const int, const std::string&, const std::string&)> new_sdc_error_handler) {
    sdc_error = new_sdc_error_handler;
}

/*
 * Class member functions
 */

bool SdcCommands::has_commands() {
    if(!create_clock_cmds.empty()) return true;
    if(!set_input_delay_cmds.empty()) return true;
    if(!set_output_delay_cmds.empty()) return true;
    if(!set_clock_groups_cmds.empty()) return true;
    if(!set_false_path_cmds.empty()) return true;
    if(!set_max_delay_cmds.empty()) return true;
    if(!set_multicycle_path_cmds.empty()) return true;
    return false; 
}

}
