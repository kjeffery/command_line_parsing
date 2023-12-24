#include <iostream>
#include <unordered_map>

template <typename T>
struct Argument
{
    std::string_view short_name;
    std::string_view long_name;
    std::string_view description;
    T                value;
};

class ArgumentBase
{
public:
    virtual ~ArgumentBase() = default;

    [[nodiscard]] virtual ArgumentBase* clone() const = 0;

    // template <typename T>
    // void add(std::string_view scommand, std::string_view lcommand, std::string_view description, )

private:
};

class SwitchArgument : public ArgumentBase
{
public:
    [[nodiscard]] SwitchArgument* clone() const override
    {
        return new SwitchArgument(*this);
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
    explicit ValueArgument(Argument<T> in);

    [[nodiscard]] ValueArgument* clone() const override
    {
        return new ValueArgument(*this);
    }

    [[nodiscard]] const T& get() const;
};

template <typename T>
class PositionalArguments : public ArgumentBase
{
public:
    [[nodiscard]] PositionalArguments* clone() const override
    {
        return new PositionalArguments(*this);
    }

    [[nodiscard]] std::size_t size() const noexcept;

    auto begin() const;
    auto end() const;
    auto cbegin() const;
    auto cend() const;
};

class CommandLineParser
{
public:
    void parse(const int argc, const char* argv[]);

    void print_help();

    void add(const ArgumentBase& argument)
    {
        // Clone
        // Add to vector
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
        const Argument<std::string> name_args = {
            .short_name = "n", .long_name = "name", .description = "User name", .value = "Marcus"
        };
        ValueArgument name{name_args};

        //ValueArgument<std::string> name{name_args};
        const auto x = name.clone();
    } catch (const std::exception& e) {
    }
}
