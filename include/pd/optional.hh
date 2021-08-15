#ifndef PD_OPTIONAL_OPTIONAL_HH_
#define PD_OPTIONAL_OPTIONAL_HH_ 
#pragma once

#include <type_traits>
#include <memory>
#include <exception>

namespace pd
{

// TAGS
struct in_place_t
{
    explicit in_place_t() = default;
};
constexpr static in_place_t in_place{};

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
        : dummy_{}, is_set_{false} {}

    template<typename... Ts>
    constexpr optional_storage_(pd::in_place_t, Ts... args)
        : value_(std::forward<Ts>(args)...), is_set_(true) {}

    ~optional_storage_()
    {
        if (is_set_)
        {
            value_.~T();
            is_set_ = false;
        }
    }

private:
    struct dummy_t{};
    union
    {
        dummy_t dummy_;
        T value_;
    };
    bool is_set_;
};

template<typename T>
struct optional_storage_<T, true>
{
    constexpr optional_storage_() noexcept 
        : dummy_{}, is_set_{false} {}

    template<typename... Ts>
    constexpr optional_storage_(pd::in_place_t, Ts... args)
        : value_(std::forward<Ts>(args)...), is_set_(true) {}

private:
    struct dummy_t{};
    union
    {
        dummy_t dummy_;
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
    
    constexpr bool is_set() const noexcept
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
        return std::move(this->value_);
    }

    constexpr const T&& get() const &&
    {
        return std::move(this->value_);
    }

    constexpr void hard_reset()
    {
        get().~T();
        this->is_set_ = false;
    }

    template<typename... Ts>
    constexpr void construct(Ts&&... args) noexcept(noexcept(T(std::forward<Ts>(args)...)))
    {
        new (std::addressof(this->value)) T(std::forward<Ts>(args)...);
        this->is_set_ = true;
    }

    template<typename Option>
    constexpr void assign(Option&& other)
    {
        if (is_set())
        {
            if (!other.is_set())
                hard_reset();
        }
        else
        {
            construct(other.get());
        }
    }
};

// handling the case where T either trivially copy constructible or not
template<typename T, bool = std::is_trivially_copy_constructible<T>::value>
struct optional_copy_ : optional_operations_<T>
{
    using optional_operations_<T>::optional_operations_;

    constexpr optional_copy_(const optional_copy_ &other)
        : optional_copy_()
    {
        if (other->is_set())
            construct(other.get());
    }

    constexpr optional_copy_() = default;
    constexpr optional_copy_(optional_copy_ &&) = default;
    constexpr optional_copy_&
    operator= (const optional_copy_ &) = default;
    constexpr optional_copy_&
    operator= (optional_copy_ &&) = default;
};

template<typename T>
struct optional_copy_<T, true> : optional_operations_<T>
{
    using optional_operations_<T>::optional_operations_;
};

// handling the case where T either trivially move constructible or not
template<typename T, bool = std::is_trivially_move_constructible<T>::value>
struct optional_move_ : optional_copy_<T>
{
    using optional_copy_<T>::optional_copy_;

    constexpr optional_move_(optional_move_ &&other) noexcept(std::is_nothrow_move_constructible<T>::value)
    {
        if (other->is_set())
            construct(std::move(other.get()));
    }

    constexpr optional_move_() = default;
    constexpr optional_move_(const optional_move_ &) = default;
    constexpr optional_move_&
    operator= (const optional_move_ &) = default;
    constexpr optional_move_&
    operator= (optional_move_ &&) = default;
};

template<typename T>
struct optional_move_<T, true> : optional_copy_<T>
{
    using optional_copy_<T>::optional_copy_;
};

// handling the case where T either trivially copy assignable or not
template<typename T, bool = std::is_trivially_destructible<T>::value &&
                            std::is_trivially_copy_constructible<T>::value &&
                            std::is_trivially_copy_assignable<T>::value>
struct optional_copy_assign_ : optional_move_<T>
{
    using optional_move_<T>::optional_move_;

    constexpr optional_copy_assign_&
    operator=(const optional_copy_assign_ &other)
    {
        assign(other);
        return *this;
    }

    constexpr optional_copy_assign_() = default;
    constexpr optional_copy_assign_(const optional_copy_assign_ &) = default;
    constexpr optional_copy_assign_(optional_copy_assign_ &&) = default;
    constexpr optional_copy_assign_&
    operator= (optional_copy_assign_ &&) = default;
};

template<typename T>
struct optional_copy_assign_<T, true> : optional_move_<T>
{
    using optional_move_<T>::optional_move_;
};

// handling the case where T either trivially move assignable or not
template<typename T, bool = std::is_trivially_destructible<T>::value &&
                            std::is_trivially_move_constructible<T>::value &&
                            std::is_trivially_move_assignable<T>::value>
struct optional_move_assign_ : optional_move_<T>
{
    using optional_move_<T>::optional_move_;

    constexpr optional_move_assign_&
    operator= (optional_move_assign_ &&other)
    {
        assign(std::move(other));
        return *this;
    }

    constexpr optional_move_assign_() = default;
    constexpr optional_move_assign_(const optional_move_assign_ &) = default;
    constexpr optional_move_assign_(optional_move_assign_ &&) = default;
    constexpr optional_move_assign_& 
    operator= (const optional_move_assign_ &) = default;
};

template<typename T>
struct optional_move_assign_<T, true> : optional_move_<T>
{
    using optional_move_<T>::optional_move_;
};

// handling the case where T cannot be copy/move constructible 
template<typename T, bool IsCopy = std::is_copy_constructible<T>::value,
                     bool IsMove = std::is_move_constructible<T>::value>
struct optional_delete_copy_or_move_ {
    constexpr optional_delete_copy_or_move_() = default;
    constexpr optional_delete_copy_or_move_(const optional_delete_copy_or_move_ &) = delete;
    constexpr optional_delete_copy_or_move_(optional_delete_copy_or_move_ &&) = delete;
    constexpr optional_delete_copy_or_move_&
    operator= (const optional_delete_copy_or_move_ &) = default;
    constexpr optional_delete_copy_or_move_&
    operator= (optional_delete_copy_or_move_ &&) = default;
};

template<typename T>
struct optional_delete_copy_or_move_<T, true, true> {
    constexpr optional_delete_copy_or_move_() = default;
    constexpr optional_delete_copy_or_move_(const optional_delete_copy_or_move_ &) = default;
    constexpr optional_delete_copy_or_move_(optional_delete_copy_or_move_ &&) = default;
    constexpr optional_delete_copy_or_move_&
    operator= (const optional_delete_copy_or_move_ &) = default;
    constexpr optional_delete_copy_or_move_&
    operator= (optional_delete_copy_or_move_ &&) = default;
};

template<typename T>
struct optional_delete_copy_or_move_<T, true, false> {
    constexpr optional_delete_copy_or_move_() = default;
    constexpr optional_delete_copy_or_move_(const optional_delete_copy_or_move_ &) = default;
    constexpr optional_delete_copy_or_move_(optional_delete_copy_or_move_ &&) = delete;
    constexpr optional_delete_copy_or_move_&
    operator= (const optional_delete_copy_or_move_ &) = default;
    constexpr optional_delete_copy_or_move_&
    operator= (optional_delete_copy_or_move_ &&) = default;
};

template<typename T>
struct optional_delete_copy_or_move_<T, false, true> {
    constexpr optional_delete_copy_or_move_() = default;
    constexpr optional_delete_copy_or_move_(const optional_delete_copy_or_move_ &) = delete;
    constexpr optional_delete_copy_or_move_(optional_delete_copy_or_move_ &&) = default;
    constexpr optional_delete_copy_or_move_&
    operator= (const optional_delete_copy_or_move_ &) = default;
    constexpr optional_delete_copy_or_move_&
    operator= (optional_delete_copy_or_move_ &&) = default;
};

// handling the case where T cannot be copy/move assignable 
template<typename T, bool IsCopy = std::is_copy_constructible<T>::value &&
                                   std::is_copy_assignable<T>::value,
                     bool IsMove = std::is_move_constructible<T>::value &&
                                   std::is_move_assignable<T>::value>
struct optional_delete_copy_or_move_assign_ {
    constexpr optional_delete_copy_or_move_assign_() = default;
    constexpr optional_delete_copy_or_move_assign_(const optional_delete_copy_or_move_assign_ &) = default;
    constexpr optional_delete_copy_or_move_assign_(optional_delete_copy_or_move_assign_ &&) = default;
    constexpr optional_delete_copy_or_move_assign_&
    operator= (const optional_delete_copy_or_move_assign_ &) = delete;
    constexpr optional_delete_copy_or_move_assign_&
    operator= (optional_delete_copy_or_move_assign_ &&) = delete;
};

template<typename T>
struct optional_delete_copy_or_move_assign_<T, true, true> {
    constexpr optional_delete_copy_or_move_assign_() = default;
    constexpr optional_delete_copy_or_move_assign_(const optional_delete_copy_or_move_assign_ &) = default;
    constexpr optional_delete_copy_or_move_assign_(optional_delete_copy_or_move_assign_ &&) = default;
    constexpr optional_delete_copy_or_move_assign_&
    operator= (const optional_delete_copy_or_move_assign_ &) = default;
    constexpr optional_delete_copy_or_move_assign_&
    operator= (optional_delete_copy_or_move_assign_ &&) = default;
};

template<typename T>
struct optional_delete_copy_or_move_assign_<T, true, false> {
    constexpr optional_delete_copy_or_move_assign_() = default;
    constexpr optional_delete_copy_or_move_assign_(const optional_delete_copy_or_move_assign_ &) = default;
    constexpr optional_delete_copy_or_move_assign_(optional_delete_copy_or_move_assign_ &&) = default;
    constexpr optional_delete_copy_or_move_assign_&
    operator= (const optional_delete_copy_or_move_assign_ &) = default;
    constexpr optional_delete_copy_or_move_assign_&
    operator= (optional_delete_copy_or_move_assign_ &&) = delete;
};

template<typename T>
struct optional_delete_copy_or_move_assign_<T, false, true> {
    constexpr optional_delete_copy_or_move_assign_() = default;
    constexpr optional_delete_copy_or_move_assign_(const optional_delete_copy_or_move_assign_ &) = default;
    constexpr optional_delete_copy_or_move_assign_(optional_delete_copy_or_move_assign_ &&) = default;
    constexpr optional_delete_copy_or_move_assign_&
    operator= (const optional_delete_copy_or_move_assign_ &) = delete;
    constexpr optional_delete_copy_or_move_assign_&
    operator= (optional_delete_copy_or_move_assign_ &&) = default;
};
} // namespace detail

struct nullopt_t 
{
    struct hidden_{};
    constexpr explicit nullopt_t(hidden_) noexcept {}
};

static constexpr nullopt_t nullopt{nullopt_t::hidden_{}};

struct bad_optional_access : public std::exception
{
    bad_optional_access() = default;

    const char* what() const noexcept
    {
        return "Optional has no value or etc";
    }
};

template<typename T>
struct optional : private detail::optional_move_assign_<T>,
                  private detail::optional_delete_copy_or_move_<T>,
                  private detail::optional_delete_copy_or_move_assign_<T>
{
private:
    using base = detail::optional_move_assign_<T>;

    static_assert(!std::is_same<in_place_t, std::decay<T>>::value, "instatiation with in_place_t is ill-formed");
    static_assert(!std::is_same<nullopt_t, std::decay<T>>::value, "instatiation with nullopt_t is ill-formed");
};

} // namespace pd

#endif // PD_OPTIONAL_OPTIONAL_HH_
