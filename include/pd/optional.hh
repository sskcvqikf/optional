#ifndef PD_OPTIONAL_OPTIONAL_HH_
#define PD_OPTIONAL_OPTIONAL_HH_ 
#pragma once

#include <type_traits>

namespace pd
{

struct nullopt_t {};
static constexpr nullopt_t nullopt{};

namespace detail
{
// optional_storage_ holds actual data and responsible
// for proper object deletion since union requires it
// two versions: one for trivial destructible object
// second for others
template<typename T, bool = std::is_trivially_destructible<T>::value>
struct optional_storage_
{
constexpr optional_storage_() noexcept 
    : nullopt_{}, is_set_{false} {}

constexpr optional_storage_(pd::nullopt_t) noexcept
    : optional_storage_() {}

~optional_storage_()
{
    if (is_set_)
    {
        value_.~T();
        is_set_ = false;
    }
}

private:
    union
    {
        nullopt_t nullopt_;
        T value_;
    };
    bool is_set_;
};

template<typename T>
struct optional_storage_<T, true>
{
constexpr optional_storage_() noexcept 
    : nullopt_{}, is_set_{false} {}

constexpr optional_storage_(pd::nullopt_t) noexcept
    : optional_storage_() {}

private:
    union
    {
        nullopt_t nullopt_;
        T value_;
    };
    bool is_set_;
};


// optional_operations_ provides some essential
// operations to derived classes
template <typename T>
struct optional_operations_ : optional_storage_<T>
{
    using optional_storage_<T>::optional_storage_; // pulling constructors
    
    constexpr const bool is_set() const noexcept
    {
        return this->is_set_;
    }

    constexpr T& get() &
    {
        return this->value_;
    }

    constexpr const T& get() const &
    {
        return this->value_;
    }

    constexpr T&& get() &&
    {
        return move(this->value_);
    }

    constexpr const T&& get() const &&
    {
        return move(this->value_);
    }

};
} // namespace detail

template<typename T>
struct optional
{


};

} // namespace pd

#endif // PD_OPTIONAL_OPTIONAL_HH_
