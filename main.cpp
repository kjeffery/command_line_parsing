#include <iostream>
#include <string_view>
#include <thread>
#include <unordered_map>

template <typename T>
struct Argument
{
    std::string_view short_name{};
    std::string_view long_name{};
    std::string_view description{};
    T                value{};
};

class SwitchArgument
{
public:
    [[nodiscard]] bool get() const noexcept
    {
        return m_value;
    }

    explicit operator bool() const noexcept
    {
        return m_value;
    }

private:
    bool m_value{ false };
};

template <typename T, std::size_t count = 1>
class ValueArgument
{
public:
    explicit ValueArgument(Argument<T> in);

    [[nodiscard]] const T& get(std::size_t idx = 0) const noexcept
    {
        return m_data[idx];
    }

private:
    Argument<T>
    std::array<T, count> m_data;
};

template <typename T>
class PositionalArguments
{
public:
    [[nodiscard]] std::size_t size() const noexcept;

    auto begin() const;
    auto end() const;
    auto cbegin() const;
    auto cend() const;

private:
    std::vector<T> m_data;
};

class CommandLineParser
{
    struct ArgumentBase
    {
        virtual      ~ArgumentBase() = default;
        virtual void read(std::istream& ins) = 0;

        //[[nodiscard]] virtual ArgumentBase* clone() const = 0;
    };

    template <typename T>
    struct TypedArgument : public ArgumentBase
    {
        std::string_view short_name;
        std::string_view long_name;
        std::string_view description;
        T                value;

        //[[nodiscard]] virtual TypedArgument* clone() const = 0;
    };

public:
    void parse(const int argc, const char* argv[])
    {
        for (int i = 1; i < argc; ++i) {
            std::string_view s(argv[i]);
            if (s.starts_with("--")) {
            } else if (s.starts_with('-')) {
            }
        }
    }

    void print_help();

    template <typename T>
    void add(const T& argument)
    {
        if ()
            // Create TypedArgument in unique_ptr
            // Copy values


    }

    // template <typename T>
    // void add(std::string_view scommand, std::string_view lcommand, std::string_view description, )

private:
    std::unordered_map<std::string_view, std::string_view>              m_short_to_long;
    std::unordered_map<std::string_view, std::unique_ptr<ArgumentBase>> m_long_to_arg;
};

// template <typename T>
// ValueArgument(Argument<T>) -> ValueArgument<T>;

int main(int argc, const char* argv[])
{
    try {
        const auto default_threads = std::thread::hardware_concurrency();

        CommandLineParser parser;

        const Argument<std::string> name_args = {
            .short_name = "n", .long_name = "name", .description = "User name", .value = "Marcus"
        };
        ValueArgument name{ name_args };

        const Argument<std::size_t> thread_args = {
            .long_name = "threads", .description = "Number of threads", .value = default_threads
        };
        ValueArgument               threads{ thread_args };

        const Argument<std::size_t, 2> resolution_args = {
            .short_name = "r", .long_name = "resolution", .description = "Number of threads", .value = default_threads
        };

        parser.add(name);
        parser.add(threads);

        parser.parse(argc, argv);

        std::println(std::cout, "Got name: {}", name.get());
    } catch (const std::exception& e) {
    }
}
