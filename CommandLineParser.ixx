export module CommandLineParser;

import std;

namespace CommandLineParser {
using ArgumentContainer = std::vector<std::string_view>;
using Iterator = ArgumentContainer::const_iterator;

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

enum class UserInput
{
    required,
    optional
};

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

export struct NamedParameterInput
{
    std::string_view short_name{};
    std::string_view long_name{};
    std::string_view description{};
    bool             user_input_required{ false };
};

class ParameterBase
{
public:
    constexpr ParameterBase(const std::string_view description,
                            const UserInput        user_input)
    : m_description{ description }
    , m_user_input{ user_input }
    {
    }

    virtual ~ParameterBase() = default;

    void read(Iterator first, Iterator last)
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
    virtual void                      read_impl(Iterator first, Iterator last) = 0;
    [[nodiscard]] virtual std::size_t get_min_arg_count_impl() const noexcept = 0;
    [[nodiscard]] virtual std::size_t get_max_arg_count_impl() const noexcept = 0;

    bool             m_set_by_user{ false };
    std::string_view m_description;
    UserInput        m_user_input;
};

class NamedParameterBase : public ParameterBase
{
public:
    constexpr NamedParameterBase(const std::string_view description,
                                 const UserInput        user_input,
                                 const std::string_view short_name,
                                 const std::string_view long_name)
    : ParameterBase{ description, user_input }
    , m_short_name{ short_name }
    , m_long_name{ long_name }
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
    constexpr explicit NamedRuntimeParameterImpl(
            U&&         value,
            std::size_t num_values_min,
            std::size_t num_values_max)
    : m_values(std::forward<U>(value))
    , m_num_values_min{ num_values_min }
    , m_num_values_max{ num_values_max }
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
        // Get rid of default values on first read.
        // TODO: multiple calls may need to check if this is the first read (use set_by_user?)
        m_values.clear();

        for (; first != last; ++first) {
            std::span<const char> span_view(*first);
            std::ispanstream      ins(span_view);

            if (!m_values.has_value()) {
                m_values = std::vector<T>{};
            }
            m_values.push_back(do_read<T>(ins));
        }
    }

    std::optional<std::vector<T>> m_values;
    std::size_t                   m_num_values_min;
    std::size_t                   m_num_values_max;
};

template <typename T, std::size_t num_values_min, std::size_t num_values_max>
struct NamedFixedParameterImpl
{
    constexpr NamedFixedParameterImpl() = default;

    template <typename U = T>
    constexpr explicit NamedFixedParameterImpl(U&& value)
    //: m_value(std::forward<U>(value))
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
        return num_values_min;
    }

    [[nodiscard]] constexpr std::size_t get_max_arg_count() const noexcept
    {
        return num_values_max;
    }

    void read(Iterator first, Iterator last)
    {
        // Get rid of default values on first read.
        // TODO: multiple calls may need to check if this is the first read (use set_by_user?)
        m_values.clear();

        for (; first != last; ++first) {
            std::span<const char> span_view(*first);
            std::ispanstream      ins(span_view);

            if (!m_values.has_value()) {
                m_values = std::vector<T>{};
            }
            m_values.push_back(do_read<T>(ins));
        }
    }

    std::optional<std::vector<T>> m_values;
};

template <typename T>
struct NamedFixedParameterImpl<T, 1ULL, 1ULL>
{
    constexpr NamedFixedParameterImpl() = default;

    template <typename U = T>
    constexpr NamedFixedParameterImpl(U&& value)
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

    void read(Iterator first, Iterator last)
    {
        std::span<const char> span_view(*first);
        std::ispanstream      ins(span_view);

        m_value = do_read<T>(ins);
    }

    std::optional<T> m_value;
};

// TODO: we probably don't need the private inheritence: just own one.
export template <typename T, std::size_t num_values_min = 1ULL, std::size_t num_values_max = num_values_min>
class NamedParameter final : public NamedParameterBase,
                             private NamedFixedParameterImpl<T, num_values_min, num_values_max>
{
    using Impl = NamedFixedParameterImpl<T, num_values_min, num_values_max>;

public:
    explicit NamedParameter(const NamedParameterInput& in)
    : NamedParameterBase(
                         in.description,
                         //(in.user_input_required) ? UserInput::required : UserInput::optional,
                         UserInput::required,
                         in.short_name,
                         in.long_name
                        )
    {
    }

private:
    [[nodiscard]] std::size_t get_min_arg_count_impl() const noexcept override
    {
        return Impl::get_min_arg_count();
    }

    [[nodiscard]] std::size_t get_max_arg_count_impl() const noexcept override
    {
        return Impl::get_max_arg_count();
    }

    void read_impl(Iterator first, Iterator last) override
    {
        Impl::read(first, last);
    }
};

// TODO: we probably don't need the private inheritence: just own one.
export template <typename T, std::size_t unused>
class NamedParameter<T, runtime_decision, unused> final : public NamedParameterBase,
                                                          private NamedRuntimeParameterImpl<T>
{
    using Impl = NamedRuntimeParameterImpl<T>;

public:
    explicit NamedParameter(const NamedParameterInput& in,
                            std::size_t                num_parameters_min,
                            std::size_t                num_parameters_max)
    : NamedParameterBase{
        in.description, (in.user_input_required) ? UserInput::required : UserInput::optional, in.short_name,
        in.long_name
    }
    , Impl(num_parameters_min, num_parameters_max)
    {
    }

private:
    [[nodiscard]] std::size_t get_min_arg_count_impl() const noexcept override
    {
        return Impl::get_min_arg_count();
    }

    [[nodiscard]] std::size_t get_max_arg_count_impl() const noexcept override
    {
        return Impl::get_max_arg_count();
    }

    void read_impl(Iterator first, Iterator last) override
    {
        Impl::read(first, last);
    }
};
} // namespace CommandLineModule
