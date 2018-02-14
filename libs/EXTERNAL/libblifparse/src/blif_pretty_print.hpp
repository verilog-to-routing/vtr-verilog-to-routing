#ifndef BLIF_PRETTY_PRINT
#define BLIF_PRETTY_PRINT
#include <cstdio>
#include "blifparse.hpp"

namespace blifparse {

//An example callback which pretty-prints to stdout
//the BLIF which is being parsed 
class BlifPrettyPrinter : public Callback {
    public:
        BlifPrettyPrinter(bool print_file_line=false)
            : print_file_line_(print_file_line) {}

        void start_parse() override;
        void filename(std::string fname) override;
        void lineno(int line_num) override;
        void begin_model(std::string model_name) override;
        void inputs(std::vector<std::string> inputs) override;
        void outputs(std::vector<std::string> outputs) override;

        void names(std::vector<std::string> nets, std::vector<std::vector<LogicValue>> so_cover) override;

        void latch(std::string input, std::string output, LatchType type, std::string control, LogicValue init) override;

        void subckt(std::string model, std::vector<std::string> ports, std::vector<std::string> nets) override;

        void blackbox() override;

        void end_model() override;

        //BLIF Extensions
        void conn(std::string src, std::string dst) override;
        void cname(std::string cell_name) override;
        void attr(std::string name, std::string value) override;
        void param(std::string name, std::string value) override;

        void finish_parse() override;

        void parse_error(const int curr_lineno, const std::string& near_text, const std::string& msg) override {
            fprintf(stderr, "Custom Error at line %d near '%s': %s\n", curr_lineno, near_text.c_str(), msg.c_str());
            had_error_ = true;
        }

        bool had_error() { return had_error_; }

    private:
        std::string indent();

        size_t indent_level_ = 0;

        std::string filename_ = "";
        int lineno_ = 0;
        bool print_file_line_ = false;
        bool had_error_ = false;
};

}
#endif
