module;

#include <istream>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

export module CommandLineParser;

namespace CommandLineParser {
export class CLParseError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

export class CLSetupError : public std::logic_error
{
public:
    using std::logic_error::logic_error;
};

template <typename... Args>
void throw_parse_error(std::format_string<Args...> fmt, Args&&... args)
{
    throw CLParseError{ std::format(fmt, std::forward<Args>(args)...) };
}

template <typename... Args>
void throw_setup_error(std::format_string<Args...> fmt, Args&&... args)
{
    throw CLSetupError{ std::format(fmt, std::forward<Args>(args)...) };
}

constexpr auto runtime_decision = std::numeric_limits<std::size_t>::max() - 1ULL;

template <typename T, typename = void>
struct has_custom_read : std::false_type
{
};

template <typename T>
struct has_custom_read<T,
                       std::void_t<decltype(parameter_read(std::declval<std::istream&>(), std::declval<T>())
                       )>> : std::true_type
{
};

template <typename T>
    requires std::is_default_constructible_v<T>
T do_read(std::istream& ins)
{
    T t;
    if constexpr (has_custom_read<T>::value) {
        parameter_read(ins, t);
    } else {
        ins >> t;
    }
    return t;
}

export struct NamedParameterInput
{
    std::string_view short_name{};
    std::string_view long_name{};
    std::string_view description{};
    bool             user_input_required{ false };
};

template <typename T>
struct NamedRuntimeParameterImpl
{
    constexpr NamedRuntimeParameterImpl() = default;

    constexpr NamedRuntimeParameterImpl(std::size_t num_values_min, std::size_t num_values_max)
    : m_num_values_min{ num_values_min }
    , m_num_values_max{ num_values_max }
    {
    }

    template <typename U = T>
    constexpr explicit(std::is_convertible_v<U&&, T>) NamedRuntimeParameterImpl(
            U&&         value,
            std::size_t num_values_min,
            std::size_t num_values_max)
    : m_value(std::forward<U>(value))
    , m_num_values_min{ num_values_min }
    , m_num_values_max{ num_values_max }
    {
    }

    template <typename U>
    [[nodiscard]] constexpr T value_or(std::size_t idx, U&& default_value) const &
    {
        return (*m_value)[idx].value_or(std::forward<U>(default_value));
    }

    [[nodiscard]] constexpr T& value(std::size_t idx)
    {
        return (*m_value)[idx].value();
    }

    [[nodiscard]] constexpr const T& value(std::size_t idx) const
    {
        return (*m_value)[idx].value();
    }

    [[nodiscard]] constexpr std::size_t get_min_arg_count() const noexcept
    {
        return m_num_values_min;
    }

    [[nodiscard]] constexpr std::size_t get_max_arg_count() const noexcept
    {
        return m_num_values_max;
    }

    std::optional<std::vector<T>> m_value;
    std::size_t                   m_num_values_min;
    std::size_t                   m_num_values_max;
};

template <typename T, std::size_t num_values_min, std::size_t num_values_max>
struct NamedFixedParameterImpl
{
    consteval NamedFixedParameterImpl() = default;

    template <typename U = T>
    consteval explicit(std::is_convertible_v<U&&, T>) NamedFixedParameterImpl(U&& value)
    //: m_value(std::forward<U>(value))
    {
    }

    template <typename U>
    [[nodiscard]] constexpr T value_or(std::size_t idx, U&& default_value) const &
    {
        return (*m_value)[idx].value_or(std::forward<U>(default_value));
    }

    [[nodiscard]] constexpr T& value(std::size_t idx)
    {
        return (*m_value)[idx].value();
    }

    [[nodiscard]] constexpr const T& value(std::size_t idx) const
    {
        return (*m_value)[idx].value();
    }

    [[nodiscard]] constexpr std::size_t get_min_arg_count() const noexcept
    {
        return num_values_min;
    }

    [[nodiscard]] constexpr std::size_t get_max_arg_count() const noexcept
    {
        return num_values_max;
    }

    std::optional<std::vector<T>> m_value;
};

template <typename T>
struct NamedFixedParameterImpl<T, 1ULL, 1ULL>
{
    consteval NamedFixedParameterImpl() = default;

    template <typename U = T>
    consteval explicit(std::is_convertible_v<U&&, T>) NamedFixedParameterImpl(U&& value)
    : m_value(std::forward<U>(value))
    {
    }

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

    std::optional<T> m_value;
};

export template <typename T, std::size_t num_values_min = 1ULL, std::size_t num_values_max = num_values_min>
class NamedParameter : private NamedFixedParameterImpl<T, num_values_min, num_values_max>
{
    using Parent = NamedFixedParameterImpl<T, num_values_min, num_values_max>;

public:
    explicit NamedParameter(const NamedParameterInput& in)
    : Parent()
    {
    }
};

export template <typename T, std::size_t unused>
class NamedParameter<T, runtime_decision, unused> : private NamedRuntimeParameterImpl<T>
{
    using Parent = NamedRuntimeParameterImpl<T>;

public:
    explicit NamedParameter(const NamedParameterInput& in,
                            std::size_t                     num_parameters_min,
                            std::size_t                     num_parameters_max)
    : Parent(num_parameters_min, num_parameters_max)
    {
    }
};
} // namespace CommandLineModule
