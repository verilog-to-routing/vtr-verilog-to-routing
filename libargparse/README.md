libargparse
===========
This is (yet another) simple command-line parser for C++ applications, inspired by Python's agparse module.

It requires only a C++11 compiler, and has no external dependancies.

One of the advantages of libargparse is that all conversions from command-line strings to program types (bool, int etc.) are performed when the command line is parsed (and not when the options are accessed).
This avoids command-line related errors from showing up deep in the program execution, which can be problematic for long-running programs.

Basic Usage
===========

```
#include "argparse.hpp"

struct Args {
    bool do_foo;
    bool enable_bar;

    std::string filename;

    size_t verbosity;
};

int main(int argc, const char** argv) {
    Args args;
    auto parser = argparse::ArgumentParser(argv[0], "My application description");

    parser.add_argument(args.filename, "filename")
        .help("File to process");

    parser.add_argument(args.do_foo, "--foo")
        .help("Causes foo")
        .default_value("false")
        .action(argparse::Action::STORE_TRUE);

    parser.add_argument(args.enable_bar, "--bar")
        .help("Control bar")
        .default_value("false");

    parser.add_argument(args.verbosity, "--verbosity", "-v")
        .help("Sets the verbosity")
        .default_value("1")
        .choices({"0", "1", "2"});

    parser.parse_args(argc, argv);

    //Show the arguments
    std::cout << "args.filename: " << args.filename << "\n";
    std::cout << "args.do_foo: " << args.do_foo << "\n";
    std::cout << "args.verbosity: " << args.verbosity << "\n";
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
```

and the resulting help:

```
$ ./argparse_example -h
usage: argparse_example filename [--foo] [--bar {true, false}] 
       [-v {0, 1, 2}] [-h]

My application description

arguments:
  filename          File to process
  --foo             Causes foo (Default: false)
  --bar {true, false}
                    Control whether bar is enabled (Default: false)
  -v {0, 1, 2}, --verbosity {0, 1, 2}
                    Sets the verbosity (Default: 1)
  -h, --help        Shows this help message
```
By default the usage and help messages are line-wrapped to 80 characters.

Custom Conversions
==================
By default libargparse performs string to program type conversions using ``<sstream>``, meaning any type supporting ``operator<<()`` and ``operator>>()`` should be automatically supported.

However this does not always provide sufficient flexibility.
As a result libargparse also supports custom conversions, allowing user-defined mappings between command-line strings to program types.

If we wanted to modify the above example so the '--bar' argument accepted the strings 'on' and 'off' (instead of the default 'true' and 'false') we would define a custom class as follows:
```
struct OnOff {
    bool from_str(std::string str) {
        if      (str == "on")  return true;
        else if (str == "off") return false;
        throw argparse::ArgParseConversionError("Invalid argument value");
    }

    std::string to_str(bool val) {
        if (val) return "on";
        return "off";
    }

    std::vector<std::string> default_choices() {
        return {"on", "off"};
    }
};
```

Where the `from_str()` and `to_str()` define the conversions to and from a string, and `default_choices()` returns the set of valid choices. Note that default_choices() can return an empty vector to indicate there is no specified default set of choices.

We then modify the ``add_argument()`` call to use our conversion object:
```
    parser.add_argument<bool,OnOff>(args.enable_bar, "--bar")
        .help("Control whether bar is enabled")
        .default_value("off");
```

with the resulting help:
```
usage: argparse_example filename [--foo] [--bar {on, off}] [-v {0, 1, 2}] 
       [-h]

My application description

arguments:
  filename          File to process
  --foo             Causes foo (Default: false)
  --bar {on, off}   Control whether bar is enabled (Default: off)
  -v {0, 1, 2}, --verbosity {0, 1, 2}
                    Sets the verbosity (Default: 1)
  -h, --help        Shows this help message
```

Advanced Usage
==============
For more advanced usage such as argument groups see [argparse_test.cpp](argparse_test.cpp) and [src/argparse.hpp](argparse.hpp)

Future Work
===========
libargparse is missing a variety of more advanced features found in Python's argparse, including (but not limited to):
* nargs: '?', '*', '+', >1
* action: append, count
* subcommands
* mutually exclusive options
* prefix abbreviations
* parsing only known args
* concatenated short options (e.g. `-xvf`, for options `-x`, `-v`, `-f`)
* equal concatenated option values (e.g. `--foo=VALUE`)

Acknowledgements
================
Python's [https://docs.python.org/2.7/library/argparse.html](argparse module)
