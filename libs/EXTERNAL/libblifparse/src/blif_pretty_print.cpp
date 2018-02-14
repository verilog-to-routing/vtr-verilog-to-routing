#include <cstdio>
#include <cassert>

#include "blif_pretty_print.hpp"

namespace blifparse {

void BlifPrettyPrinter::start_parse() {
    //Pass
}

void BlifPrettyPrinter::finish_parse() {
    //Pass
}

void BlifPrettyPrinter::begin_model(std::string model_name) {
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".model %s\n", model_name.c_str());
}

void BlifPrettyPrinter::inputs(std::vector<std::string> input_conns) {
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".inputs \\\n");
    ++indent_level_;
    for(size_t i = 0; i < input_conns.size(); ++i) {
        printf("%s%s", indent().c_str(), input_conns[i].c_str());
        if(i != input_conns.size() - 1) {
            printf(" \\");
        }
        printf("\n");
    }
    --indent_level_;
}

void BlifPrettyPrinter::outputs(std::vector<std::string> output_conns) {
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".outputs \\\n");

    ++indent_level_;
    for(size_t i = 0; i < output_conns.size(); ++i) {
        printf("%s%s", indent().c_str(), output_conns[i].c_str());
        if(i != output_conns.size() - 1) {
            printf(" \\");
        }
        printf("\n");
    }
    --indent_level_;
}

void BlifPrettyPrinter::names(std::vector<std::string> nets, std::vector<std::vector<LogicValue>> so_cover) {
    printf("\n");
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".names ");
    for(size_t i = 0; i < nets.size(); ++i) {
        printf("%s", nets[i].c_str());
        if(i != nets.size() - 1) {
            printf(" ");
        }
    }
    printf("\n");

    for(const auto& so_row : so_cover) { 
        for(size_t i = 0; i < so_row.size(); ++i) {
            switch(so_row[i]) {
                case LogicValue::FALSE:     printf("0"); break;
                case LogicValue::TRUE:      printf("1"); break;
                case LogicValue::DONT_CARE: printf("-"); break;
                default: assert(false);
            }
            if(i == so_row.size() - 2) {
                printf(" ");
            }
        }
        printf("\n");
    }
}

void BlifPrettyPrinter::latch(std::string input, std::string output, LatchType type, std::string control, LogicValue init) {
    printf("\n");
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".latch \\\n");

    ++indent_level_;
    printf("%s%s \\\n", indent().c_str(), input.c_str());
    printf("%s%s \\\n", indent().c_str(), output.c_str());
    switch(type) {
        case LatchType::RISING_EDGE:    printf("%sre \\\n", indent().c_str()); break;
        case LatchType::FALLING_EDGE:   printf("%sfe \\\n", indent().c_str()); break;
        case LatchType::ACTIVE_HIGH:    printf("%sah \\\n", indent().c_str()); break;
        case LatchType::ACTIVE_LOW:     printf("%sal \\\n", indent().c_str()); break;
        case LatchType::ASYNCHRONOUS:   printf("%sas \\\n", indent().c_str()); break;
        case LatchType::UNSPECIFIED:    break;
        default: assert(false);

    }
    if(control.empty()) {
        printf("%sNIL \\\n", indent().c_str());
    } else {
        printf("%s%s \\\n", indent().c_str(), control.c_str());
    }
    switch(init) {
        case LogicValue::FALSE:     printf("%s0", indent().c_str()); break;
        case LogicValue::TRUE:      printf("%s1", indent().c_str()); break;
        case LogicValue::DONT_CARE: printf("%s2", indent().c_str()); break;
        case LogicValue::UNKOWN:    printf("%s3", indent().c_str()); break;
        default: assert(false);
    }
    --indent_level_;
}

void BlifPrettyPrinter::subckt(std::string model, std::vector<std::string> ports, std::vector<std::string> nets) {
    printf("\n");
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".subckt %s \\\n", model.c_str());

    ++indent_level_;
    assert(ports.size() == nets.size());
    for(size_t i = 0; i < ports.size(); i++) {
        printf("%s%s=%s", indent().c_str(), ports[i].c_str(), nets[i].c_str());

        if(i != ports.size() - 1) {
            printf(" \\");
        }

        printf("\n");
    }
    --indent_level_;
}

void BlifPrettyPrinter::blackbox() {
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".blackbox\n");
}

void BlifPrettyPrinter::end_model() {
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".end\n");
}

void BlifPrettyPrinter::conn(std::string src, std::string dst) {
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".conn %s %s\n", src.c_str(), dst.c_str());

}

void BlifPrettyPrinter::cname(std::string cell_name) {
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".cname %s\n", cell_name.c_str());
}

void BlifPrettyPrinter::attr(std::string name, std::string value) {
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".attr %s %s\n", name.c_str(), value.c_str());
}

void BlifPrettyPrinter::param(std::string name, std::string value) {
    if(print_file_line_) {
        printf("#%s:%d\n", filename_.c_str(), lineno_);
    }
    printf(".param %s %s\n", name.c_str(), value.c_str());
}

void BlifPrettyPrinter::filename(std::string fname) {
    filename_ = fname;
}

void BlifPrettyPrinter::lineno(int line_num) {
    lineno_ = line_num;
}

std::string BlifPrettyPrinter::indent() {
    std::string indent_str;
    for(size_t i = 0; i < indent_level_; ++i) {
        indent_str += "    ";
    }
    return indent_str;
}

}
