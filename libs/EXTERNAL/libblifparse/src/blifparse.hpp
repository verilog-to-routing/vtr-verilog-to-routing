#ifndef BLIFPARSE_H
#define BLIFPARSE_H
/*
 * libblifparse - Kevin E. Murray 2016
 *
 * Released under MIT License see LICENSE.txt for details.
 *
 * OVERVIEW
 * --------------------------
 * This library provides support for parsing Berkely Logic Interchange Format (BLIF)
 * files. It supporst the features required to handle basic netlists (e.g. .model, 
 * .inputs, .outputs, .subckt, .names, .latch)
 *
 * USAGE
 * --------------------------
 * Define a callback derived from the blifparse::Callback interface, and pass it
 * to one of the blifparse::blif_parse_*() functions.
 *
 * The parser will then call the various callback methods as it encouters the 
 * appropriate parts of the netlist.
 *
 * See main.cpp and blif_pretty_print.hpp for example usage.
 *
 */
#include <vector>
#include <string>
#include <memory>
#include <limits>
#include <functional>

namespace blifparse {
/*
 * Data structure Forward declarations
 */
enum class LogicValue;
enum class LatchType;

class Callback {
    public:
        virtual ~Callback() {}

        //Start of parsing
        virtual void start_parse() = 0;

        //Sets current filename
        virtual void filename(std::string fname) = 0;

        //Sets current line number
        virtual void lineno(int line_num) = 0;

        //Start of a .model
        virtual void begin_model(std::string model_name) = 0;

        //.inputs
        virtual void inputs(std::vector<std::string> inputs) = 0;

        //.outputs
        virtual void outputs(std::vector<std::string> outputs) = 0;

        //.names
        virtual void names(std::vector<std::string> nets, std::vector<std::vector<LogicValue>> so_cover) = 0;

        //.latch
        virtual void latch(std::string input, std::string output, LatchType type, std::string control, LogicValue init) = 0;

        //.subckt
        virtual void subckt(std::string model, std::vector<std::string> ports, std::vector<std::string> nets) = 0;

        //.blackbox
        virtual void blackbox() = 0;

        //.end (of a .model)
        virtual void end_model() = 0;

        //.conn [Extended BLIF, produces an error if not overriden]
        virtual void conn(std::string src, std::string dst);

        //.cname [Extended BLIF, produces an error if not overriden]
        virtual void cname(std::string cell_name);

        //.attr [Extended BLIF, produces an error if not overriden]
        virtual void attr(std::string name, std::string value);

        //.param [Extended BLIF, produces an error if not overriden]
        virtual void param(std::string name, std::string value);

        //End of parsing
        virtual void finish_parse() = 0;

        //Error during parsing
        virtual void parse_error(const int curr_lineno, const std::string& near_text, const std::string& msg) = 0;
};


/*
 * External functions for loading an SDC file
 */
void blif_parse_filename(std::string filename, Callback& callback);
void blif_parse_filename(const char* filename, Callback& callback);

//Loads from 'blif'. 'filename' only used to pass a filename to callback and can be left unspecified
void blif_parse_file(FILE* blif, Callback& callback, const char* filename=""); 

/*
 * Enumerations
 */
enum class LogicValue {
    FALSE = 0,  //Logic zero
    TRUE = 1,   //Logic one
    DONT_CARE,     //Don't care
    UNKOWN  //Unkown (e.g. latch initial state)
};

enum class LatchType {
    FALLING_EDGE,
    RISING_EDGE,
    ACTIVE_HIGH,
    ACTIVE_LOW,
    ASYNCHRONOUS,
    UNSPECIFIED //If no type is specified
};

} //namespace

#endif
