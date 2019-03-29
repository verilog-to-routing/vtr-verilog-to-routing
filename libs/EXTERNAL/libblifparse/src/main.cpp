#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "blifparse.hpp"
#include "blif_pretty_print.hpp"

using namespace blifparse;

int exit_code = 0;

class NoOpCallback : public Callback {
    //A No-op version of the callback
    public:
        void start_parse() override {}

        void filename(std::string /*fname*/) override {}
        void lineno(int /*line_num*/) override {}

        void begin_model(std::string /*model_name*/) override {}
        void inputs(std::vector<std::string> /*inputs*/) override {}
        void outputs(std::vector<std::string> /*outputs*/) override {}

        void names(std::vector<std::string> /*nets*/, std::vector<std::vector<LogicValue>> /*so_cover*/) override {}
        void latch(std::string /*input*/, std::string /*output*/, LatchType /*type*/, std::string /*control*/, LogicValue /*init*/) override {}
        void subckt(std::string /*model*/, std::vector<std::string> /*ports*/, std::vector<std::string> /*nets*/) override {}
        void blackbox() override {}

        void end_model() override {}

        void finish_parse() override {}

        void parse_error(const int curr_lineno, const std::string& near_text, const std::string& msg) override {
            fprintf(stderr, "Custom Error at line %d near '%s': %s\n", curr_lineno, near_text.c_str(), msg.c_str());
            had_error_ = true;
        }

        bool had_error() { return had_error_ = true; }

    private:
        bool had_error_ = false;
};

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s filename.blif\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "Reads in an blif file into internal data structures\n");
        fprintf(stderr, "and then prints it out\n");
        exit(1);
    }

    //Parse the file
    blifparse::BlifPrettyPrinter callback(true);
    //NoOpCallback callback;
    blif_parse_filename(argv[1], callback);

    if(callback.had_error()) {
        return 1;
    } else {
        return 0;
    }
}
