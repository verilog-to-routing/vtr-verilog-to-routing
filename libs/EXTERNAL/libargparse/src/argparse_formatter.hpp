#ifndef ARGPARSE_FORMATTER_HPP
#define ARGPARSE_FORMATTER_HPP
#include <string>

namespace argparse {

    class ArgumentParser;

    class Formatter {
        public:
            virtual ~Formatter() {}
            virtual void set_parser(ArgumentParser* parser) = 0;
            virtual std::string format_usage() const = 0;
            virtual std::string format_description() const = 0;
            virtual std::string format_arguments() const = 0;
            virtual std::string format_epilog() const = 0;
            virtual std::string format_version() const = 0;
    };

    class DefaultFormatter : public Formatter {
        public:
            DefaultFormatter(size_t option_arg_width=20, size_t total_width=80);
            void set_parser(ArgumentParser* parser) override;
            std::string format_usage() const override;
            std::string format_description() const override;
            std::string format_arguments() const override;
            std::string format_epilog() const override;
            std::string format_version() const override;
        private:
            size_t option_name_width_;
            size_t total_width_;
            ArgumentParser* parser_;
    };

} //namespace
#endif
