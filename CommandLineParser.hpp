#pragma once

#include "Finally.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <format>
#include <iostream>
#include <istream>
#include <limits>
#include <map>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#if __has_include(<print>)
#include <print>
#endif

#if __has_include(<spanstream>)
#include <spanstream>
#else
#include <sstream>
#endif

namespace CommandLineParser
{
using ArgumentContainer = std::vector<std::string_view>;
using Iterator = ArgumentContainer::const_iterator;

class CLError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class CLParseError : public CLError
{
public:
    using CLError::CLError;
};

class CLSetupError : public CLError
{
public:
    using CLError::CLError;
};

template <typename... Args>
void throw_parse_error(std::format_string<Args...> fmt, Args&&... args)
{
    throw CLParseError{std::format(fmt, std::forward<Args>(args)...)};
}

template <typename... Args>
void throw_setup_error(std::format_string<Args...> fmt, Args&&... args)
{
    throw CLSetupError{std::format(fmt, std::forward<Args>(args)...)};
}

constexpr auto runtime_decision = std::numeric_limits<std::size_t>::max() - 1ULL;

enum class UserInput
{
    required,
    optional
};

template <typename T>
concept string_like = std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>;

template <typename T>
concept string_like_input_iterator = std::input_iterator<T> && string_like<typename std::iterator_traits<
    T>::value_type>;

template <typename T>
concept has_custom_parameter_read = requires(T a)
{
    custom_parameter_read(std::declval<std::istream&>(), a);
};

template <typename T>
    requires std::is_default_constructible_v<T>
T do_read(std::istream& ins)
{
    T t;
    if constexpr (has_custom_parameter_read<T>) {
        custom_parameter_read(ins, t);
    } else {
        ins >> t;
    }
    return t;
}

inline auto make_input_stream(std::string_view sv) -> decltype(auto)
{
#if defined(__cpp_lib_spanstream)
    std::span<const char> span_view{sv.data(), sv.size()};
    return std::ispanstream(span_view);
#else
    return std::istringstream(std::string(sv));
#endif
}

struct NamedParameterInput
{
    std::string_view short_name{};
    std::string_view long_name{};
    std::string_view description{};
    bool user_input_required{false};
};

class ParameterBase
{
public:
    constexpr ParameterBase(const std::string_view description,
                            const UserInput user_input)
    : m_description{description}
    , m_user_input{user_input}
    {
    }

    virtual ~ParameterBase() = default;

    void read(const Iterator first, const Iterator last)
    {
        read_impl(first, last);
        m_set_by_user = true;
    }

    [[nodiscard]] constexpr std::string_view get_description() const noexcept
    {
        return m_description;
    }

    [[nodiscard]] std::size_t get_min_arg_count() const noexcept
    {
        return get_min_arg_count_impl();
    }

    [[nodiscard]] std::size_t get_max_arg_count() const noexcept
    {
        return get_max_arg_count_impl();
    }

    [[nodiscard]] constexpr bool set_by_user() const noexcept
    {
        return m_set_by_user;
    }

    [[nodiscard]] constexpr bool is_required() const noexcept
    {
        return m_user_input == UserInput::required;
    }

    [[nodiscard]] bool is_variable_length() const noexcept
    {
        return get_min_arg_count() != get_max_arg_count();
    }

private:
    virtual void read_impl(Iterator first, Iterator last) = 0;
    [[nodiscard]] virtual std::size_t get_min_arg_count_impl() const noexcept = 0;
    [[nodiscard]] virtual std::size_t get_max_arg_count_impl() const noexcept = 0;

    bool m_set_by_user{false};
    std::string_view m_description;
    UserInput m_user_input;
};

class NamedParameterBase : public ParameterBase
{
public:
    constexpr NamedParameterBase(const std::string_view description,
                                 const UserInput user_input,
                                 const std::string_view short_name,
                                 const std::string_view long_name)
    : ParameterBase{description, user_input}
    , m_short_name{short_name}
    , m_long_name{long_name}
    {
    }

    [[nodiscard]] constexpr std::string_view get_short_name() const noexcept
    {
        return m_short_name;
    }

    [[nodiscard]] constexpr std::string_view get_long_name() const noexcept
    {
        return m_long_name;
    }

private:
    std::string_view m_short_name;
    std::string_view m_long_name;
};

class PositionalParameterBase : public ParameterBase
{
public:
    constexpr PositionalParameterBase(const std::string_view description,
                                      const UserInput user_input)
    : ParameterBase{description, user_input}
    {
    }
};

template <typename T>
struct NamedRuntimeParameterImpl
{
    constexpr NamedRuntimeParameterImpl(std::size_t num_values_min, std::size_t num_values_max)
    : m_num_values_min{num_values_min}
    , m_num_values_max{num_values_max}
    {
    }

    template <typename U = T>
    constexpr explicit NamedRuntimeParameterImpl(
        U&& value,
        std::size_t num_values_min,
        std::size_t num_values_max)
    : m_values(std::forward<U>(value))
    , m_num_values_min{num_values_min}
    , m_num_values_max{num_values_max}
    {
    }

    template <typename U>
    [[nodiscard]] constexpr T value_or(std::size_t idx, U&& default_value) const &
    {
        return (*m_values)[idx].value_or(std::forward<U>(default_value));
    }

    [[nodiscard]] constexpr T& value(std::size_t idx)
    {
        return (*m_values)[idx].value();
    }

    [[nodiscard]] constexpr const T& value(std::size_t idx) const
    {
        return (*m_values)[idx].value();
    }

    [[nodiscard]] constexpr std::size_t get_min_arg_count() const noexcept
    {
        return m_num_values_min;
    }

    [[nodiscard]] constexpr std::size_t get_max_arg_count() const noexcept
    {
        return m_num_values_max;
    }

    void read(Iterator first, Iterator last)
    {
        // Reset values on each read (duplicate flag policy: last occurrence wins).
        m_values.reset();

        for (; first != last; ++first) {
            auto ins = make_input_stream(*first);
            if (!m_values) {
                m_values = std::vector<T>{};
            }
            m_values->push_back(do_read<T>(ins));
        }
    }

    std::optional<std::vector<T>> m_values;
    std::size_t m_num_values_min;
    std::size_t m_num_values_max;
};

template <typename T, std::size_t num_values_min, std::size_t num_values_max>
struct NamedFixedParameterImpl
{
    template <typename U>
    [[nodiscard]] constexpr T value_or(std::size_t idx, U&& default_value) const &
    {
        return (*m_values)[idx].value_or(std::forward<U>(default_value));
    }

    [[nodiscard]] constexpr T& value(std::size_t idx)
    {
        return (*m_values)[idx].value();
    }

    [[nodiscard]] constexpr const T& value(std::size_t idx) const
    {
        return (*m_values)[idx].value();
    }

    [[nodiscard]] constexpr std::size_t get_min_arg_count() const noexcept
    {
        return num_values_min;
    }

    [[nodiscard]] constexpr std::size_t get_max_arg_count() const noexcept
    {
        return num_values_max;
    }

    void read(Iterator first, Iterator last)
    {
        // Reset values on each read (duplicate flag policy: last occurrence wins).
        m_values.reset();

        for (; first != last; ++first) {
            auto ins = make_input_stream(*first);
            if (!m_values) {
                m_values = std::vector<T>{};
            }
            m_values->push_back(do_read<T>(ins));
        }
    }

    std::optional<std::vector<T>> m_values;
};

template <typename T>
struct NamedFixedParameterImpl<T, 1ULL, 1ULL>
{
    template <typename U>
    [[nodiscard]] constexpr T value_or(U&& default_value) const &
    {
        return m_value.value_or(std::forward<U>(default_value));
    }

    template <typename U>
    [[nodiscard]] constexpr T value_or(U&& default_value) &&
    {
        return std::move(m_value).value_or(std::forward<U>(default_value));
    }

    [[nodiscard]] constexpr T& value() &
    {
        return m_value.value();
    }

    [[nodiscard]] constexpr const T& value() const &
    {
        return m_value.value();
    }

    [[nodiscard]] constexpr T&& value() &&
    {
        return std::move(m_value).value();
    }

    [[nodiscard]] constexpr const T&& value() const &&
    {
        return std::move(m_value).value();
    }

    [[nodiscard]] constexpr std::size_t get_min_arg_count() const noexcept
    {
        return 1;
    }

    [[nodiscard]] constexpr std::size_t get_max_arg_count() const noexcept
    {
        return 1;
    }

    void read(Iterator first, Iterator last)
    {
        if (first == last) {
            throw_parse_error("Missing value for parameter");
        }
        auto ins = make_input_stream(*first);
        m_value = do_read<T>(ins);
    }

    std::optional<T> m_value;
};

template <typename T, std::size_t num_values_min = 1ULL, std::size_t num_values_max = num_values_min>
class NamedParameter final : public NamedParameterBase
{
    using Impl = NamedFixedParameterImpl<T, num_values_min, num_values_max>;

public:
    explicit NamedParameter(const NamedParameterInput& in)
    : NamedParameterBase(
                         in.description,
                         (in.user_input_required) ? UserInput::required : UserInput::optional,
                         in.short_name,
                         in.long_name
                        )
    {
    }

    // Accessors
    template <typename U>
    [[nodiscard]] constexpr T value_or(std::size_t idx, U&& default_value) const &
    {
        return m_impl.value_or(idx, std::forward<U>(default_value));
    }

    [[nodiscard]] constexpr T& value(std::size_t idx)
    {
        return m_impl.value(idx);
    }

    [[nodiscard]] constexpr const T& value(std::size_t idx) const
    {
        return m_impl.value(idx);
    }

private:
    [[nodiscard]] std::size_t get_min_arg_count_impl() const noexcept override
    {
        return m_impl.get_min_arg_count();
    }

    [[nodiscard]] std::size_t get_max_arg_count_impl() const noexcept override
    {
        return m_impl.get_max_arg_count();
    }

    void read_impl(Iterator first, Iterator last) override
    {
        m_impl.read(first, last);
    }

    Impl m_impl;
};

template <typename T, std::size_t unused>
class NamedParameter<T, runtime_decision, unused> final : public NamedParameterBase
{
    using Impl = NamedRuntimeParameterImpl<T>;

public:
    explicit NamedParameter(const NamedParameterInput& in,
                            std::size_t num_parameters_min,
                            std::size_t num_parameters_max)
    : NamedParameterBase{
        in.description, (in.user_input_required) ? UserInput::required : UserInput::optional, in.short_name,
        in.long_name
    }
    , m_impl(num_parameters_min, num_parameters_max)
    {
    }

    // Accessors
    template <typename U>
    [[nodiscard]] constexpr T value_or(std::size_t idx, U&& default_value) const &
    {
        return m_impl.value_or(idx, std::forward<U>(default_value));
    }

    [[nodiscard]] constexpr T& value(std::size_t idx)
    {
        return m_impl.value(idx);
    }

    [[nodiscard]] constexpr const T& value(std::size_t idx) const
    {
        return m_impl.value(idx);
    }

private:
    [[nodiscard]] std::size_t get_min_arg_count_impl() const noexcept override
    {
        return m_impl.get_min_arg_count();
    }

    [[nodiscard]] std::size_t get_max_arg_count_impl() const noexcept override
    {
        return m_impl.get_max_arg_count();
    }

    void read_impl(Iterator first, Iterator last) override
    {
        m_impl.read(first, last);
    }

    Impl m_impl;
};

[[nodiscard]] inline std::size_t get_number_available_sub_args(Iterator first, Iterator last)
{
    // Precondition: first does not point to the leading argument.
    // E.g. for "--resolution", "800", "600" we pass in "800", "600"
    std::size_t count{0};
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
    void parse(Iterator first, const Iterator last)
    {
        // Precondition: the executable name has been removed from argc
        while (first != last) {
            if (const auto& arg = *first; arg == "--") {
                // Do positional parsing
                // Don't mark an empty positional as an error here...maybe it was blank for a reason.
                break;
            } else if (arg.starts_with("--")) {
                auto long_name = arg;
                long_name.remove_prefix(2); // Remove "--"

                const auto it = m_long_to_short.find(long_name);
                if (it == m_long_to_short.end()) {
                    throw_parse_error("Not a valid argument: --{}", long_name);
                }

                const auto& short_name = it->second;

                auto* obj = m_args.at(Key{short_name, long_name});
                assert(obj);

                first = parse_named_parameter_sub_arguments(*obj, first, last);
            } else if (arg.starts_with('-')) {
                auto short_name = arg;
                short_name.remove_prefix(1); // Remove '-'

                const auto it = m_short_to_long.find(short_name);
                if (it == m_short_to_long.end()) {
                    throw_parse_error("Not a valid argument: -{}", short_name);
                }
                const auto& long_name = it->second;

                auto* obj = m_args.at(Key{short_name, long_name});
                assert(obj);

                first = parse_named_parameter_sub_arguments(*obj, first, last);
            } else {
                if (!m_positional) {
                    throw_parse_error("There are leftover arguments that could not be parsed");
                }
                break;
            }
        }
        if (m_positional) {
            parse_positionals(*m_positional, first, last);
        }
        // Validate required parameters.
        for (auto&& [_, arg] : m_args) {
            assert(arg);
            if (arg->is_required() && !arg->set_by_user()) {
                throw_parse_error("Required flag '{}' was not provided", get_representation_name(*arg));
            }
        }
        if (m_positional && m_positional->is_required() && !m_positional->set_by_user()) {
            throw_parse_error("Required positional argument '{}' was not provided", m_positional->get_description());
        }
    }

    // If we send non-random-access iterators or iterators with std::string, we convert to random-access string_view
    // iterators.
    void parse(string_like_input_iterator auto first, string_like_input_iterator auto last)
    {
        ArgumentContainer string_views;
        std::ranges::transform(first,
                               last,
                               std::back_inserter(string_views),
                               [](auto s)
                               {
                                   return std::string_view(s);
                               }
                              );
        parse(string_views.cbegin(), string_views.cend());
    }

    void print_help(std::ostream& outs, std::string_view program_name) const
    {
        const bool ambiguous = is_ambiguous();
        const bool has_optional_named = true;
        const bool has_required_named = true;
        const bool has_required_positionals = false;
        const bool has_optional_positionals = true;

        assert(!has_optional_positionals || !has_required_positionals);

        std::print(outs, "Usage: {}", program_name);
        if (has_required_named) {
            std::print(outs, " <required flags>");
        }
        if (has_optional_named) {
            std::print(outs, " [optional flags]");
        }
        if (ambiguous) {
            std::print(outs, " --");
        }
        if (has_required_positionals) {
            assert(m_positional);
            std::print(outs, " <{}>", m_positional->get_description());
        } else if (has_optional_positionals) {
            assert(m_positional);
            std::print(outs, " [{}]", m_positional->get_description());
        }

        auto is_required = [](auto x)
        {
            return x.second->is_required();
        };
        auto is_not_required = [](auto x)
        {
            return !x.second->is_required();
        };
        std::ranges::for_each(m_args | std::ranges::views::filter(is_required),
                              [&outs](auto x)
                              {
                                  std::println(outs, "Required: {}", get_representation_name(*x.second));
                              });
        std::ranges::for_each(m_args | std::ranges::views::filter(is_not_required),
                              [&outs](auto x)
                              {
                                  std::println(outs, "Optional: {}", get_representation_name(*x.second));
                              });
    }

    void add(PositionalParameterBase& parameter)
    {
        if (m_positional) {
            throw_setup_error("Positional arguments specified more than once");
        }
        m_positional = std::addressof(parameter);
    }

    void add(NamedParameterBase& parameter)
    {
        const auto short_name = parameter.get_short_name();
        const auto long_name = parameter.get_long_name();

        if (short_name.empty() && long_name.empty()) {
            throw_setup_error("Argument type requires at least one of a short name or a long name");
        }

        if (!short_name.empty()) {
            if (auto [iter, inserted] = m_short_to_long.emplace(short_name, long_name); !inserted) {
                throw_setup_error("Short name {} already specified", short_name);
            }
        }

        if (!long_name.empty()) {
            if (auto [iter, inserted] = m_long_to_short.emplace(long_name, short_name); !inserted) {
                throw_setup_error("Long name {} already specified", long_name);
            }
        }

        // We add empty names deliberately.
        m_args.emplace(Key{short_name, long_name}, std::addressof(parameter));
    }

private:
    template <string_like_input_iterator Iterator>
    auto parse_named_parameter_sub_arguments(NamedParameterBase& obj,
                                             Iterator first,
                                             Iterator last) -> Iterator
    {
        const auto min_num_sub_arguments = obj.get_min_arg_count();
        const auto num_available_sub_arguments = get_number_available_sub_args(first + 1, last);

        if (num_available_sub_arguments < min_num_sub_arguments) {
            throw_parse_error("Fewer arguments ({}) specified than required ({}) for flag {}",
                              num_available_sub_arguments,
                              min_num_sub_arguments,
                              get_representation_name(obj));
        }

        const auto max_num_sub_arguments = obj.get_max_arg_count();
        const auto number_to_read = std::min(max_num_sub_arguments, num_available_sub_arguments);
        obj.read(first + 1, first + 1 + number_to_read); // Skip over this value
        return first + 1 + number_to_read;
    }

    void parse_positionals(PositionalParameterBase& obj,
                           string_like_input_iterator auto first,
                           string_like_input_iterator auto last)
    {
        const auto min_num_sub_arguments = obj.get_min_arg_count();
        const auto num_available_sub_arguments = get_number_available_sub_args(first, last);

        if (num_available_sub_arguments < min_num_sub_arguments) {
            throw_parse_error("Fewer arguments ({}) specified than required ({}) for positional arguments",
                              num_available_sub_arguments,
                              min_num_sub_arguments);
        }

        const auto max_num_sub_arguments = obj.get_max_arg_count();
        if (num_available_sub_arguments > max_num_sub_arguments) {
            throw_parse_error("More arguments ({}) specified than required ({}) for positional arguments",
                              num_available_sub_arguments,
                              max_num_sub_arguments);
        }

        obj.read(first, last);
    }

    [[nodiscard]] bool is_ambiguous() const
    {
        if (!m_positional) {
            return false;
        }

        // We can resolve parsing exactly _n_ positional arguments
        if (!m_positional->is_variable_length() && m_positional->is_required()) {
            return false;
        }

        for (auto&& [_, arg] : m_args) {
            assert(arg);
            if (arg->is_variable_length()) {
                // At this point, we have a variadic positional (either we don't know the number, or the number is
                // optional, which comes out to the same thing), and so a variadic named argument conflicts with this.
                return true;
            }
        }
        return false;
    }

    static std::string_view get_representation_name(const NamedParameterBase& arg)
    {
        const auto& long_name = arg.get_long_name();
        return (long_name.empty()) ? arg.get_short_name() : long_name;
    }

    using ShortName = std::string_view;
    using LongName = std::string_view;
    using Key = std::pair<ShortName, LongName>;

    std::map<ShortName, LongName> m_short_to_long;
    std::map<LongName, ShortName> m_long_to_short;
    std::map<Key, NamedParameterBase*> m_args;
    PositionalParameterBase* m_positional{nullptr};
};

ArgumentContainer argv_to_string_views(const std::span<const char*> sub_argv)
{
    std::vector<std::string_view> result;
    result.reserve(sub_argv.size());
    for (auto s : sub_argv) {
        result.emplace_back(s);
    }
    return result;
}
} // namespace CommandLineParser
