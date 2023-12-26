#pragma once

#include <utility>

template <typename F>
class Finally
{
public:
    explicit Finally(F f)
    : m_f(std::move(f))
    {
    }

    ~Finally() noexcept
    {
        m_f();
    }

private:
    F m_f;
};

template <typename F>
Finally<F> finally(F f)
{
    return Finally<F>(std::move(f));
}