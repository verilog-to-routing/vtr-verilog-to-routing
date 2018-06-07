#include "argparse.hpp"

using argparse::ArgValue;
using argparse::ConvertedValue;

struct Args {
    ArgValue<bool> do_foo;
    ArgValue<bool> enable_bar;
    ArgValue<std::string> filename;
    ArgValue<size_t> verbosity;
    ArgValue<bool> show_version;
    ArgValue<float> utilization;
    ArgValue<std::vector<float>> zulus;
    ArgValue<std::vector<float>> alphas;
};

struct OnOff {
    ConvertedValue<bool> from_str(std::string str) {
        ConvertedValue<bool> converted_value;

        if      (str == "on")  converted_value.set_value(true);
        else if (str == "off") converted_value.set_value(false);
        else                   converted_value.set_error("Invalid argument value");
        return converted_value;
    }

    ConvertedValue<std::string> to_str(bool val) {
        ConvertedValue<std::string> converted_value;
        if (val) converted_value.set_value("on");
        else     converted_value.set_value("off");
        return converted_value;
    }

    std::vector<std::string> default_choices() {
        return {"on", "off"};
    }
};

struct ZeroOneRange {
    ConvertedValue<float> from_str(std::string str) {
        float value;
        std::stringstream ss(str);
        ss >> value;

        ConvertedValue<float> converted_value;
        if (value < 0. || value > 1.) {
            std::stringstream msg;
            msg << "Value '" << value << "' out of expected range [0.0, 1.0]";
            converted_value.set_error(msg.str());
        } else {
            converted_value.set_value(value);
        }
        return converted_value;
    }

    std::string to_str(float value) {
        return std::to_string(value);
    }

    std::vector<std::string> default_choices() {
        return {};
    }
};

int main(int argc, const char** argv) {
    Args args;
    auto parser = argparse::ArgumentParser(argv[0], "My application description");

    parser.version("Version: 0.0.1");

    parser.add_argument(args.filename, "filename")
        .help("File to process");

    parser.add_argument(args.do_foo, "--foo")
        .help("Causes foo")
        .default_value("false")
        .action(argparse::Action::STORE_TRUE);

    parser.add_argument<bool,OnOff>(args.enable_bar, "--bar")
        .help("Control whether bar is enabled")
        .default_value("off");

    parser.add_argument(args.verbosity, "--verbosity", "-v")
        .help("Sets the verbosity")
        .default_value("1")
        .choices({"0", "1", "2"});
    parser.add_argument(args.show_version, "--version", "-V")
        .help("Show version information")
        .action(argparse::Action::VERSION);

    parser.add_argument<float,ZeroOneRange>(args.utilization, "--util")
        .help("Sets target utilization")
        .default_value("1.0");

    parser.add_argument(args.zulus, "--zulu")
        .help("One or more float values")
        .nargs('+')
        .default_value({"1.0", "0.2"});

    parser.add_argument(args.alphas, "--alphas")
        .help("Zero or more float values")
        .nargs('*')
        .default_value({});

    parser.parse_args(argc, argv);

    //Show the arguments
    std::cout << "args.filename: " << args.filename << "\n";
    std::cout << "args.do_foo: " << args.do_foo << "\n";
    std::cout << "args.verbosity: " << args.verbosity << "\n";
    std::cout << "args.utilization: " << args.utilization << "\n";
    std::cout << "args.zulu: " << argparse::join(args.zulus.value(), ", ") << "\n";
    std::cout << "args.alphas: " << argparse::join(args.alphas.value(), ", ") << "\n";
    std::cout << "\n";

    //Do work
    if (args.do_foo) {
        if (args.verbosity > 0) {
            std::cout << "Doing foo with " << args.filename << "\n";
        }
        if (args.verbosity > 1) {
            std::cout << "Doing foo step 1" << "\n";
            std::cout << "Doing foo step 2" << "\n";
            std::cout << "Doing foo step 3" << "\n";
        }
    }

    if (args.enable_bar) {
        if (args.verbosity > 0) {
            std::cout << "Bar is enabled" << "\n";
        }
    } else {
        if (args.verbosity > 0) {
            std::cout << "Bar is disabled" << "\n";
        }
    }

    return 0;
}

