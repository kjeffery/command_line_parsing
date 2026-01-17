#include "CommandLineParser.hpp"

#include <cassert>
#include <thread>

class MyNonStreamableClass
{
public:
    int x{ 42 };
};

void custom_parameter_read(std::istream& ins, MyNonStreamableClass& c)
{
    int val;
    ins >> val;
    c.x = val;
}

int main(int argc, const char* argv[])
{
    using namespace std::literals;

    try {
        const auto default_threads = std::thread::hardware_concurrency();

        CommandLineParser::NamedParameter<std::string> name{
            {
                .short_name = "n", .long_name = "name",
                .description = "First name", .user_input_required = true
            }
        };

        CommandLineParser::NamedParameter<MyNonStreamableClass> non_streamable{
            {
                .short_name = "ns", .long_name = "non_streamable",
                .description = "Test case", .user_input_required = false
            }
        };

        CommandLineParser::CommandLineParser parser;
        parser.add(name);
        parser.add(non_streamable);

        const auto arg_sv = CommandLineParser::argv_to_string_views({ argv, static_cast<std::size_t>(argc) });
        parser.parse(arg_sv.begin() + 1, arg_sv.end());
    } catch (const CommandLineParser::CLSetupError& e) {
        std::cerr << "Setup error: " << e.what() << '\n';
        assert(false);
    } catch (const CommandLineParser::CLParseError& e) {
        std::cerr << "Argument error: " << e.what() << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    } catch (...) {
        std::cerr << "Unknown error\n";
    }
}
