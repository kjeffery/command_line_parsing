#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
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

struct SingleValueArgumentParams
{
    std::string_view short_name{};
    std::string_view long_name{};
    std::string_view description{};
    bool             user_input_required{ false };
};

struct ListValueArgumentParams
{
    std::string_view short_name{};
    std::string_view long_name{};
    std::string_view description{};
    std::size_t      min_values{ 1 };
    std::size_t      max_values{ 1 };
    bool             user_input_required{ false };
};

struct CountingArgumentParams
{
    std::string_view short_name{};
    std::string_view long_name{};
    std::string_view description{};
    bool             user_input_required{ false };
};

struct SwitchArgumentParams
{
    std::string_view short_name{};
    std::string_view long_name{};
    std::string_view description{};
    bool             user_input_required{ false };
};

struct SinglePositionalArgumentParams
{
    std::string_view description{};
    bool             user_input_required{ false };
};

struct ListPositionalArgumentParams
{
    std::string_view description{};
    std::size_t      min_values{ 0 };
    std::size_t      max_values{ std::numeric_limits<std::size_t>::max() };
    bool             user_input_required{ false };
};

struct ArgumentParams
{
    explicit ArgumentParams(SingleValueArgumentParams p) noexcept
    : short_name{ std::move(p.short_name) }
    , long_name{ std::move(p.long_name) }
    , description{ std::move(p.description) }
    , min_values{ 1 }
    , max_values{ 1 }
    , user_input_required{ p.user_input_required }
    {
    }

    explicit ArgumentParams(ListValueArgumentParams p) noexcept
    : short_name{ std::move(p.short_name) }
    , long_name{ std::move(p.long_name) }
    , description{ std::move(p.description) }
    , min_values{ p.min_values }
    , max_values{ p.max_values }
    , user_input_required{ p.user_input_required }
    {
    }

    explicit ArgumentParams(CountingArgumentParams p) noexcept
    : short_name{ std::move(p.short_name) }
    , long_name{ std::move(p.long_name) }
    , description{ std::move(p.description) }
    , min_values{ 0 }
    , max_values{ 0 }
    , user_input_required{ p.user_input_required }
    {
    }

    explicit ArgumentParams(SwitchArgumentParams p) noexcept
    : short_name{ std::move(p.short_name) }
    , long_name{ std::move(p.long_name) }
    , description{ std::move(p.description) }
    , min_values{ 0 }
    , max_values{ 0 }
    , user_input_required{ p.user_input_required }
    {
    }

    explicit ArgumentParams(SinglePositionalArgumentParams p) noexcept
    : short_name{}
    , long_name{}
    , description{ std::move(p.description) }
    , min_values{ 1 }
    , max_values{ 1 }
    , user_input_required{ p.user_input_required }
    {
    }

    explicit ArgumentParams(ListPositionalArgumentParams p) noexcept
    : short_name{}
    , long_name{}
    , description{ std::move(p.description) }
    , min_values{ p.min_values }
    , max_values{ p.max_values }
    , user_input_required{ p.user_input_required }
    {
    }

    std::string_view short_name;
    std::string_view long_name;
    std::string_view description;
    std::size_t      min_values;
    std::size_t      max_values;
    bool             user_input_required;
};

class ArgumentBase
{
public:
    explicit ArgumentBase(ArgumentParams p)
    : m_params{ std::move(p) }
    {
    }

    virtual ~ArgumentBase() = default;

    void read(Iterator first, Iterator last)
    {
        read_impl(first, last);
        m_set_by_user = true;
    }

    [[nodiscard]] bool is_positional() const
    {
        return is_positional_impl();
    }

    [[nodiscard]] std::string_view get_short_name() const noexcept
    {
        return m_params.short_name;
    }

    [[nodiscard]] std::string_view get_long_name() const noexcept
    {
        return m_params.long_name;
    }

    [[nodiscard]] std::string_view get_description() const noexcept
    {
        return m_params.description;
    }

    [[nodiscard]] std::size_t get_min_arg_count() const noexcept
    {
        return m_params.min_values;
    }

    [[nodiscard]] std::size_t get_max_arg_count() const noexcept
    {
        return m_params.max_values;
    }

    [[nodiscard]] bool set_by_user() const noexcept
    {
        return m_set_by_user;
    }

private:
    virtual void               read_impl(Iterator first, Iterator last) = 0;
    [[nodiscard]] virtual bool is_positional_impl() const = 0;

    bool           m_set_by_user{ false };
    ArgumentParams m_params;
};

class SwitchArgument final : public ArgumentBase
{
public:
    explicit SwitchArgument(SwitchArgumentParams a)
    : ArgumentBase{ ArgumentParams{ std::move(a) } }
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
    void read_impl(Iterator, Iterator) override
    {
        m_value = true;
    }

    [[nodiscard]] bool is_positional_impl() const override
    {
        return false;
    }

    bool m_value{ false };
};

class CountingArgument final : public ArgumentBase
{
public:
    explicit CountingArgument(CountingArgumentParams a)
    : ArgumentBase{ ArgumentParams{ std::move(a) } }
    {
    }

    [[nodiscard]] std::size_t get() const noexcept
    {
        return m_value;
    }

private:
    void read_impl(Iterator, Iterator) override
    {
        ++m_value;
    }

    [[nodiscard]] bool is_positional_impl() const override
    {
        return false;
    }

    std::size_t m_value{ 0 };
};

template <typename T>
class SingleValueArgument final : public ArgumentBase
{
public:
    explicit SingleValueArgument(SingleValueArgumentParams p, T default_value = T{})
    : ArgumentBase{ ArgumentParams{ std::move(p) } }
    , m_value{ std::move(default_value) }
    {
    }

    [[nodiscard]] const T& get() const noexcept
    {
        return m_value;
    }

private:
    void read_impl(Iterator first, Iterator last) override
    {
        assert(std::distance(first, last) == 1);
#if defined(__cpp_lib_spanstream)
        std::span<const char> span_view(*first);
        std::ispanstream      ins(span_view);
#elif defined(__cpp_lib_sstream_from_string_view)
        std::istringstream ins(s);
#else
        std::istringstream ins(std::string(s));
#endif

        ins >> m_value;
    }

    [[nodiscard]] bool is_positional_impl() const override
    {
        return false;
    }

    T m_value;
};

template <typename T>
class ListValueArgument final : public ArgumentBase
{
public:
    explicit ListValueArgument(ListValueArgumentParams p, std::initializer_list<T> default_values = {})
    : ArgumentBase{ ArgumentParams{ std::move(p) } }
    , m_values{ default_values.begin(), default_values.end() }
    {
    }

    [[nodiscard]] const T& get(std::size_t idx) const noexcept
    {
        return m_values[idx];
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_values.size();
    }

    bool is_variable_length() const noexcept
    {
        return get_min_arg_count() != get_max_arg_count();
    }

    auto begin() const
    {
        return m_values.cbegin();
    }

    auto end() const
    {
        return m_values.cend();
    }

    auto cbegin() const
    {
        return m_values.cbegin();
    }

    auto cend() const
    {
        return m_values.cend();
    }

private:
    void read_impl(Iterator first, Iterator last) override
    {
        if (!this->set_by_user()) {
            // Get rid of default values on first read.
            m_values.clear();
        }
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
            m_values.push_back(std::move(t));
        }
    }

    [[nodiscard]] bool is_positional_impl() const override
    {
        return false;
    }

    std::vector<T> m_values;
};

template <typename T>
class SinglePositionalArgument final : public ArgumentBase
{
public:
    explicit SinglePositionalArgument(SinglePositionalArgumentParams p, T default_value = T{})
    : ArgumentBase{ ArgumentParams{ std::move(p) } }
    , m_value{ std::move(default_value) }
    {
    }

    [[nodiscard]] const T& get() const noexcept
    {
        return m_value;
    }

private:
    void read_impl(Iterator first, Iterator last) override
    {
        assert(std::distance(first, last) == 1);
#if defined(__cpp_lib_spanstream)
        std::span<const char> span_view(*first);
        std::ispanstream      ins(span_view);
#elif defined(__cpp_lib_sstream_from_string_view)
        std::istringstream ins(s);
#else
        std::istringstream ins(std::string(s));
#endif

        ins >> m_value;
    }

    [[nodiscard]] bool is_positional_impl() const override
    {
        return true;
    }

    T m_value;
};

template <typename T>
class ListPositionalArguments final : public ArgumentBase
{
public:
    explicit ListPositionalArguments(ListPositionalArgumentParams p, std::initializer_list<T> default_values = {})
    : ArgumentBase{ ArgumentParams{ std::move(p) } }
    , m_values{ default_values.begin(), default_values.end() }
    {
    }

    [[nodiscard]] const T& get(std::size_t idx) const noexcept
    {
        return m_values[idx];
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_values.size();
    }

    bool is_variable_length() const noexcept
    {
        return get_min_arg_count() != get_max_arg_count();
    }

    auto begin() const
    {
        return m_values.cbegin();
    }

    auto end() const
    {
        return m_values.cend();
    }

    auto cbegin() const
    {
        return m_values.cbegin();
    }

    auto cend() const
    {
        return m_values.cend();
    }

private:
    void read_impl(Iterator first, Iterator last) override
    {
        if (!this->set_by_user()) {
            // Get rid of default values on first read.
            m_values.clear();
        }
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
            m_values.push_back(std::move(t));
        }
    }

    [[nodiscard]] bool is_positional_impl() const override
    {
        return true;
    }

    std::vector<T> m_values;
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
                break;
            } else if (arg.starts_with("--")) {
                auto long_name = arg;
                long_name.remove_prefix(2); // Remove "--"
                const auto short_name = m_long_to_short.at(long_name);

                auto* obj = m_args.at(Key{ short_name, long_name });
                assert(obj);

                first = parse_sub_arguments(*obj, first, last);
            } else if (arg.starts_with('-')) {
                auto short_name = arg;
                short_name.remove_prefix(1); // Remove '-'
                const auto long_name = m_short_to_long.at(short_name);

                auto* obj = m_args.at(Key{ short_name, long_name });
                assert(obj);

                first = parse_sub_arguments(*obj, first, last);
            }
        }
        if (m_positional) {
            parse_positionals(*m_positional, first, last);
        }
    }

    void print_help(std::string_view program_name);

    void add(ArgumentBase& argument)
    {
        if (argument.is_positional()) {
            if (m_positional) {
                throw 82; // TODO: already specified
            }
            m_positional = std::addressof(argument);
            return;
        }

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
    Iterator parse_sub_arguments(ArgumentBase& obj, Iterator first, Iterator last)
    {
        const auto min_num_sub_arguments       = obj.get_min_arg_count();
        const auto num_available_sub_arguments = get_number_available_sub_args(first + 1, last);

        if (num_available_sub_arguments < min_num_sub_arguments) {
            throw 28;
        }

        const auto max_num_sub_arguments = obj.get_max_arg_count();
        const auto number_to_read        = std::min(max_num_sub_arguments, num_available_sub_arguments);
        obj.read(first + 1, first + 1 + number_to_read); // Skip over this value
        return first + 1 + number_to_read;
    }

    void parse_positionals(ArgumentBase& obj, Iterator first, Iterator last)
    {
        const auto min_num_sub_arguments       = obj.get_min_arg_count();
        const auto num_available_sub_arguments = get_number_available_sub_args(first, last);

        if (num_available_sub_arguments < min_num_sub_arguments) {
            throw 882;
        }

        const auto max_num_sub_arguments = obj.get_max_arg_count();
        if (num_available_sub_arguments > max_num_sub_arguments) {
            throw 74;
        }

        obj.read(first, last);
    }

    using ShortName = std::string_view;
    using LongName = std::string_view;
    using Key = std::pair<ShortName, LongName>;

    std::map<ShortName, LongName> m_short_to_long;
    std::map<LongName, ShortName> m_long_to_short;
    std::map<Key, ArgumentBase*>  m_args;
    ArgumentBase*                 m_positional{ nullptr };
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

int main(int argc, const char* argv[])
{
    using namespace std::literals;
    try {
        const auto default_threads = std::thread::hardware_concurrency();

        CommandLineParser parser;

        const SingleValueArgumentParams name_args = {
            .short_name = "n", .long_name = "name", .description = "User name"
        };
        SingleValueArgument<std::string> name{ name_args, "Marcus" };

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
    } catch (const std::exception& e) {
    }
}
