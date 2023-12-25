#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <ostream>
#include <span>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

#if __has_include(<spanstream>)
#    include <spanstream>
#endif

using ArgumentContainer = std::vector<std::string_view>;
using Iterator = ArgumentContainer::const_iterator;

template <typename T>
struct Argument
{
    std::string_view short_name{};
    std::string_view long_name{};
    std::string_view description{};
    std::vector<T>   value{};
    std::size_t      min_values{ 1 };
    std::size_t      max_values{ 1 };
    bool             user_input_required{ false };
};

class ArgumentBase
{
public:
    virtual ~ArgumentBase() = default;

    void read(Iterator first, Iterator last)
    {
        read_impl(first, last);
    }

    [[nodiscard]] std::string_view get_short_name() const noexcept
    {
        return get_short_name_impl();
    }

    [[nodiscard]] std::string_view get_long_name() const noexcept
    {
        return get_long_name_impl();
    }

    [[nodiscard]] std::string_view get_description() const noexcept
    {
        return get_description_impl();
    }

    [[nodiscard]] std::size_t get_count() const noexcept
    {
        return get_count_impl();
    }

    [[nodiscard]] std::size_t get_min_arg_count() const noexcept
    {
        return get_min_arg_count_impl();
    }

    [[nodiscard]] std::size_t get_max_arg_count() const noexcept
    {
        return get_max_arg_count_impl();
    }

private:
    virtual void read_impl(Iterator first, Iterator last) = 0;

    [[nodiscard]] virtual std::string_view get_short_name_impl() const noexcept = 0;
    [[nodiscard]] virtual std::string_view get_long_name_impl() const noexcept = 0;
    [[nodiscard]] virtual std::string_view get_description_impl() const noexcept = 0;
    [[nodiscard]] virtual std::size_t      get_count_impl() const noexcept { return 1; }
    [[nodiscard]] virtual std::size_t      get_min_arg_count_impl() const noexcept { return 1; }
    [[nodiscard]] virtual std::size_t      get_max_arg_count_impl() const noexcept { return 1; }
};

class SwitchArgument : public ArgumentBase
{
public:
    void read_impl(Iterator, Iterator) override
    {
        m_value = true;
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

template <typename T>
class ValueArgument : public ArgumentBase
{
public:
    explicit ValueArgument(Argument<T> in)
    : m_data(std::move(in))
    {
    }

    void read_impl(Iterator first, Iterator last) override
    {
        if (!m_set_by_user) {
            // Get rid of default values on first read.
            m_data.value.clear();
        }
        m_set_by_user = true;
        for (; first != last; ++first) {
#if defined(__cpp_lib_spanstream)
            std::span<const char> span_view(*first);
            std::ispanstream      ins(span_view);
#elif defined(__cpp_lib_sstream_from_string_view)
    std::istringstream ins(s);
#else
    std::istringstream ins(std::string(s));
#endif

            T t;
            ins >> t;
            m_data.value.push_back(std::move(t));
        }
    }

    [[nodiscard]] const T& get(std::size_t idx = 0) const noexcept
    {
        return m_data.value[idx];
    }

    [[nodiscard]] std::string_view get_short_name_impl() const noexcept override
    {
        return m_data.short_name;
    }

    [[nodiscard]] std::string_view get_long_name_impl() const noexcept override
    {
        return m_data.long_name;
    }

    [[nodiscard]] std::string_view get_description_impl() const noexcept override
    {
        return m_data.description;
    }

    [[nodiscard]] std::size_t get_count_impl() const noexcept override
    {
        return m_data.value.size();
    }

    [[nodiscard]] bool set_by_user() const noexcept
    {
        return m_set_by_user;
    }

    [[nodiscard]] std::size_t get_min_arg_count_impl() const noexcept override
    {
        return m_data.min_values;
    }

    [[nodiscard]] std::size_t get_max_arg_count_impl() const noexcept override
    {
        return m_data.max_values;
    }

private:
    bool        m_set_by_user{ false };
    Argument<T> m_data;
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

std::size_t get_number_available_sub_args(Iterator first, Iterator last)
{
    // Precondition: first does not point to the leading argument.
    // E.g. for "--resolution", "800", "600" we pass in "800", "600"
    std::size_t count{ 0 };
    for (; first != last; ++first) {
        if (first->starts_with('-')) {
            return count;
        }
        ++count;
    }
    return count;
}

class CommandLineParser
{
public:
    void parse(const ArgumentContainer& argc)
    {
        // Precondition: the executable name has been removed from argc
        auto first = argc.cbegin();
        auto last  = argc.cend();

        while (first != last) {
            const std::string_view& arg = *first;
            if (arg == "--") {
                // Do positional parsing
            } else if (arg.starts_with("--")) {
                auto long_name = arg;
                long_name.remove_prefix(2); // Remove "--"
                const auto short_name = m_long_to_short.at(long_name);

                auto* obj = m_args.at(Key{ short_name, long_name });

                first = parse_sub_arguments(obj, first, last);
            } else if (arg.starts_with('-')) {
                auto short_name = arg;
                short_name.remove_prefix(1); // Remove '-'
                const auto long_name = m_short_to_long.at(short_name);

                auto* obj = m_args.at(Key{ short_name, long_name });

                first = parse_sub_arguments(obj, first, last);
            } // TODO: else positionals
        }
    }

    void print_help(std::string_view program_name);

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
    Iterator parse_sub_arguments(ArgumentBase* obj, Iterator first, Iterator last)
    {
        const auto min_num_sub_arguments       = obj->get_min_arg_count();
        const auto num_available_sub_arguments = get_number_available_sub_args(first + 1, last);

        if (num_available_sub_arguments < min_num_sub_arguments) {
            throw 82;
        }

        const auto max_num_sub_arguments = obj->get_max_arg_count();
        const auto number_to_read        = std::min(max_num_sub_arguments, num_available_sub_arguments);
        obj->read(first + 1, first + 1 + number_to_read); // Skip over this value
        return first + 1 + number_to_read;
    }

    using ShortName = std::string_view;
    using LongName = std::string_view;
    using Key = std::pair<ShortName, LongName>;

    std::map<ShortName, LongName> m_short_to_long;
    std::map<LongName, ShortName> m_long_to_short;
    std::map<Key, ArgumentBase*>  m_args;
};

std::vector<std::string_view> to_string_views(std::span<const char*> sub_argv)
{
    std::vector<std::string_view> result;
    result.reserve(sub_argv.size());
    for (auto s: sub_argv) {
        result.emplace_back(s);
    }
    return result;
}

std::vector<std::string_view> to_string_views_discard_first(std::span<const char*> argv);

int main(int argc, const char* argv[])
{
    using namespace std::literals;
    try {
        const auto default_threads = std::thread::hardware_concurrency();

        CommandLineParser parser;

        const Argument<std::string> name_args = {
            .short_name = "n", .long_name = "name", .description = "User name", .value = { "Marcus"s }
        };
        ValueArgument name{ name_args };

        const Argument<std::size_t> thread_args = {
            .long_name = "threads", .description = "Number of threads", .value = { default_threads }
        };
        ValueArgument threads{ thread_args };

        const Argument<std::size_t> resolution_args = {
            .short_name = "r", .long_name = "resolution", .description = "Number of threads", .value = { 800, 600 },
            .min_values = 2, .max_values = 2
        };

        ValueArgument resolution{ resolution_args };

        parser.add(name);
        parser.add(threads);
        parser.add(resolution);

        std::string_view program_name{ argv[0] };
        const auto       arg_list = to_string_views(std::span{ argv + 1, static_cast<std::size_t>(argc) - 1 });
        parser.parse(arg_list);

        std::println(std::cout,
                     "Got name: {} threads: {}, resolution {}x{}",
                     name.get(),
                     threads.get(),
                     resolution.get(0),
                     resolution.get(1));
    } catch (const std::exception& e) {
    }
}
