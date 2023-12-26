#include "CommandLineParser.hpp"

#include <filesystem>
#include <iostream>
#include <ostream>
#include <span>
#include <thread>
#include <vector>

int main(int argc, const char* argv[])
{
    using namespace std::literals;
    try {
        const auto default_threads = std::thread::hardware_concurrency();

        CommandLineParser parser;

#if 0
        const SingleValueArgumentParams name_args = {
            .short_name = "n", .long_name = "name", .description = "User name"
        };
        SingleValueArgument<std::string> name{ name_args, "Marcus" };
#endif
        SingleValueArgument<std::string> name{
            {
                .short_name = "n",
                .long_name = "name",
                .description = "User name"
            },
            "Marcus"
        };

        const SingleValueArgumentParams thread_args = {
            .long_name = "threads", .description = "Number of threads"
        };
        SingleValueArgument<std::size_t> threads{ thread_args, default_threads };

        const ListValueArgumentParams resolution_args = {
            .short_name = "r", .long_name = "resolution", .description = "Number of threads",
            .min_values = 2,
            .max_values = 2
        };
        ListValueArgument<std::size_t> resolution{ resolution_args, { 800, 600 } };

        const CountingArgumentParams verbosity_args = {
            .short_name = "v", .description = "Verbosity level"
        };
        CountingArgument verbosity{ verbosity_args };

        const SwitchArgumentParams;
        const SinglePositionalArgumentParams;
        const ListPositionalArgumentParams positional_args = {
            .description = "File names"
        };
        ListPositionalArguments<std::filesystem::path> files{ positional_args };

        parser.add(name);
        parser.add(threads);
        parser.add(resolution);
        parser.add(verbosity);
        parser.add(files);

        std::string_view program_name{ argv[0] };
        const auto       arg_list = to_string_views(std::span{ argv + 1, static_cast<std::size_t>(argc) - 1 });
        parser.parse(arg_list);

        std::println(std::cout,
                     "Got name: {} threads: {}, resolution: {}x{}, verbosity: {}",
                     name.get(),
                     threads.get(),
                     resolution.get(0),
                     resolution.get(1),
                     verbosity.get());

        for (const auto& f: files) {
            std::println(std::cout, "File: {}", f.string());
        }
    } catch (const CLSetupError& e) {
        std::cerr << "Setup error: " << e.what() << '\n';
        assert(false);
    } catch (const CLParseError& e) {
        std::cerr << "Argument error: " << e.what() << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    } catch (...) {
        std::cerr << "Unknown error\n";
    }
}
