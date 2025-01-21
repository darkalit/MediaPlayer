#pragma once

namespace Utils
{
    winrt::hstring DurationToString(uint64_t duration);

    struct DeferDummy {};

    template <typename F>
    struct Deferrer
    {
        F func;
        ~Deferrer()
        {
            func();
        }
    };

    template <typename F>
    Deferrer<F> operator*(DeferDummy, F func)
    {
        return { func };
    }

#define DEFER_(x) _defer_##x
#define DEFER(x) DEFER_(x)
#define defer auto DEFER(__COUNTER__) = Utils::DeferDummy{} *[&]()
}
