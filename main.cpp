#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <thread>
#include <unordered_map>

template <typename T>
constexpr T xorshiftr(const T& n, std::size_t i) noexcept
{
    return n ^ (n >> i);
}

template <typename T>
constexpr T xorshiftl(const T& n, std::size_t i) noexcept
{
    return n ^ (n << i);
}

constexpr std::uint32_t distribute(const std::uint32_t n) noexcept
{
    constexpr std::uint32_t p = 0x55555555UL; // pattern of alternating 0 and 1
    constexpr std::uint32_t c = 0xf479498fUL; // random uneven integer constant;
    return c * xorshiftl(p * xorshiftr(n, 16UL), 16UL);
}

constexpr std::uint64_t distribute(const std::uint64_t n) noexcept
{
    constexpr std::uint64_t p = 0x5555555555555555ULL; // pattern of alternating 0 and 1
    constexpr std::uint64_t c = 0xfb44cdcb591a2a9dULL; // random uneven integer constant;
    return c * xorshiftl(p * xorshiftr(n, 32ULL), 32ULL);
}

// Adapted from https://stackoverflow.com/a/50978188
template <typename T, typename... Rest>
void hash_combine(std::size_t& seed, const T& v, const Rest&... rest)
{
    seed = std::rotl(seed, std::numeric_limits<std::size_t>::digits / 3) ^ distribute(std::hash<T>{}(v));
    (hash_combine(seed, rest), ...);
}

template <typename T, typename U>
struct std::hash<std::pair<T, U>>
{
    std::size_t operator()(const std::pair<T, U>& p) const
    {
        std::size_t seed{ 162758582 };
        hash_combine(seed, std::hash<T>{}(p.first), std::hash<U>{}(p.second));
        return seed;
    }
};

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
    virtual void read(std::istream& ins) = 0;

    [[nodiscard]] virtual std::string_view get_short_name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view get_long_name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view get_description() const noexcept = 0;
};

class SwitchArgument : public ArgumentBase
{
public:
    void read(std::istream& ins) override
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

    void read(std::istream& ins) override
    {
        m_set_by_user = true;
        for (std::size_t i = 0; i < count; ++i) {
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
    std::vector<T> m_data;
};

class CommandLineParser
{
public:
    void parse(const int argc, const char* argv[])
    {
        for (int i = 1; i < argc; ++i) {
            std::string_view s(argv[i]);
            if (s.starts_with("--")) {
                // Read into saved argument
            } else if (s.starts_with('-')) {
            }
        }
    }

    void print_help();

    void add(ArgumentBase& argument)
    {
        // TODO: check for duplicate names
        Key key{ argument.get_short_name(), argument.get_long_name() };
        m_args.emplace(key, std::addressof(argument));
    }

private:
    using ShortName = std::string_view;
    using LongName = std::string_view;
    using Key = std::pair<ShortName, LongName>;

    //std::unordered_map<ShortName, LongName> m_short_to_long;
    std::unordered_map<Key, ArgumentBase*> m_args;
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

        std::println(std::cout, "Got name: {}", name.get());
    } catch (const std::exception& e) {
    }
}
