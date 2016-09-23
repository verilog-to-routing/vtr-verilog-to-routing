#ifndef BLIF_PRETTY_PRINT
#define BLIF_PRETTY_PRINT
#include "blifparse.hpp"

namespace blifparse {

//An example callback which pretty-prints to stdout
//the BLIF which is being parsed 
class BlifPrettyPrinter : public Callback {
    public:
        BlifPrettyPrinter(bool print_file_line=false)
            : print_file_line_(print_file_line) {}

        void start_model(std::string model_name) override;
        void inputs(std::vector<std::string> inputs) override;
        void outputs(std::vector<std::string> outputs) override;

        void names(std::vector<std::string> nets, std::vector<std::vector<LogicValue>> so_cover) override;

        void latch(std::string input, std::string output, LatchType type, std::string control, LogicValue init) override;

        void subckt(std::string model, std::vector<std::string> ports, std::vector<std::string> nets) override;

        void blackbox() override;

        void end_model() override;

        void filename(std::string fname) override;
        void lineno(int line_num) override;

    private:
        std::string indent();

        size_t indent_level_ = 0;

        std::string filename_ = "";
        int lineno_ = 0;
        bool print_file_line_ = false;
};

}
#endif
