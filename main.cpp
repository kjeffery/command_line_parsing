import std;
import CommandLineParser;

#include <cassert>

int main(int argc, const char* argv[])
{
    using namespace std::literals;
    try {
        const auto default_threads = std::thread::hardware_concurrency();

        // CommandLineParser::NamedParameter<std::string> name{
        //     CommandLineParser::NamedParameterInput{
        //         .short_name = "n", .long_name = "name",
        //         .description = "First name", .user_input_required = true
        //     }
        // };
        CommandLineParser::NamedParameter<std::string> name{
            {
                .short_name = "n", .long_name = "name",
                .description = "First name", .user_input_required = true
            }
        };
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
