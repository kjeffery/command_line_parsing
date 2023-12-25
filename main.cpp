#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string_view>
#include <thread>

template <typename T, std::size_t size = 1>
struct Argument
{
    std::string_view    short_name{};
    std::string_view    long_name{};
    std::string_view    description{};
    std::array<T, size> value{};
};

class ArgumentBase
{
public:
    virtual      ~ArgumentBase() = default;
    virtual void read(const char* argv[]) = 0;

    [[nodiscard]] virtual std::string_view get_short_name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view get_long_name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view get_description() const noexcept = 0;
    [[nodiscard]] virtual std::size_t      get_count() const noexcept { return 1; }
};

class SwitchArgument : public ArgumentBase
{
public:
    void read(const char* argv[]) override
    {
    }

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
class ValueArgument : public ArgumentBase
{
public:
    explicit ValueArgument(Argument<T, count> in)
    : m_data(std::move(in))
    {
    }

    void read(const char* argv[]) override
    {
        m_set_by_user = true;
        for (std::size_t i = 0; i < count; ++i) {
            // TODO: use buffered stream
            std::istringstream ins(argv[i]);
            ins >> m_data.value[i];
        }
    }

    [[nodiscard]] const T& get(std::size_t idx = 0) const noexcept
    {
        return m_data.value[idx];
    }

    [[nodiscard]] std::string_view get_short_name() const noexcept override
    {
        return m_data.short_name;
    }

    [[nodiscard]] std::string_view get_long_name() const noexcept override
    {
        return m_data.long_name;
    }

    [[nodiscard]] std::string_view get_description() const noexcept override
    {
        return m_data.description;
    }

    [[nodiscard]] std::size_t get_count() const noexcept override
    {
        return count;
    }

    [[nodiscard]] bool set_by_user() const noexcept
    {
        return m_set_by_user;
    }

private:
    bool               m_set_by_user{ false };
    Argument<T, count> m_data;
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
    //std::vector<T> m_data;
};

class CommandLineParser
{
public:
    void parse(const int argc, const char* argv[])
    {
        for (int i = 1; i < argc; ++i) {
            const std::string_view s(argv[i]);
            if (s.starts_with("--")) {
                auto long_name = s;
                long_name.remove_prefix(2); // Remove "--"
                const auto short_name = m_long_to_short.at(long_name);

                auto* obj = m_args.at(Key{ short_name, long_name });

                const auto num_sub_arguments = obj->get_count();

                // TODO: check bounds

                obj->read(argv + i + 1); // Skip over this value
                i += num_sub_arguments;
            } else if (s.starts_with('-')) {
                auto short_name = s;
                short_name.remove_prefix(1); // Remove '-'
                const auto long_name = m_short_to_long.at(short_name);

                auto* obj = m_args.at(Key{ short_name, long_name });

                ++i; // We've read this argument
                const auto num_sub_arguments = obj->get_count();

                // TODO: check bounds

                obj->read(argv + i + 1); // Skip over this value
                i += num_sub_arguments;
            } // TODO: else positionals
        }
    }

    void print_help();

    void add(ArgumentBase& argument)
    {
        const auto short_name = argument.get_short_name();
        const auto long_name  = argument.get_long_name();

        if (short_name.empty() && long_name.empty()) {
            throw 3; // TODO: replace At least one name must be specified
        }

        if (!short_name.empty()) {
            if (auto [iter, inserted] = m_short_to_long.emplace(short_name, long_name); !inserted) {
                throw 8; // TODO: already exists
            }
        }

        if (!long_name.empty()) {
            if (auto [iter, inserted] = m_long_to_short.emplace(long_name, short_name); !inserted) {
                throw 9; // TODO: already exists
            }
        }
        m_args.emplace(Key{ short_name, long_name }, std::addressof(argument));
    }

private:
    using ShortName = std::string_view;
    using LongName = std::string_view;
    using Key = std::pair<ShortName, LongName>;

    std::map<ShortName, LongName> m_short_to_long;
    std::map<LongName, ShortName> m_long_to_short;
    std::map<Key, ArgumentBase*>  m_args;
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
        ValueArgument threads{ thread_args };

        const Argument<std::size_t, 2> resolution_args = {
            .short_name = "r", .long_name = "resolution", .description = "Number of threads", .value = { 800, 600 }
        };

        ValueArgument resolution{ resolution_args };

        parser.add(name);
        parser.add(threads);
        parser.add(resolution);

        parser.parse(argc, argv);

        std::println(std::cout,
                     "Got name: {} threads: {}, resolution {}x{}",
                     name.get(),
                     threads.get(),
                     resolution.get(0),
                     resolution.get(1));
    } catch (const std::exception& e) {
    }
}
