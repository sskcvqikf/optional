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


// Useful types
template<bool V, typename T = void>
using enable_if_t = typename std::enable_if<V, T>::type;



// optional_storage_ holds actual data and responsible
// for proper object deletion since union requires it
// two versions: one for trivial destructible object
// second for others
template<typename T, bool = std::is_trivially_destructible<T>::value>
struct optional_storage_
{
    constexpr optional_storage_() noexcept 
        : dummy_{}, is_set_{false} {}

    template<typename... Args>
    constexpr optional_storage_(pd::in_place_t, Args&&... args)
        : value_(std::forward<Args>(args)...), is_set_(true) {}

    ~optional_storage_()
    {
        if (is_set_)
        {
            value_.~T();
            is_set_ = false;
        }
    }

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

    template<typename... Args>
    constexpr optional_storage_(pd::in_place_t, Args&&... args)
        : value_(std::forward<Args>(args)...), is_set_(true) {}

    struct dummy_t{};
    union
    {
        dummy_t dummy_;
        T value_;
    };
    bool is_set_;
};


// optional_operations_ provides some essential
// operations to derived templatees
template <typename T>
struct optional_operations_ : optional_storage_<T>
{
    using optional_storage_<T>::optional_storage_; // pulling constructors
    
    constexpr bool has_value() const noexcept
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

    template<typename... Args>
    constexpr void construct(Args&&... args) noexcept(noexcept(T(std::forward<Args>(args)...)))
    {
        new (std::addressof(this->value_)) T(std::forward<Args>(args)...);
        this->is_set_ = true;
    }

    template<typename Option>
    constexpr void assign(Option&& other)
    {
        if (has_value())
        {
            if (!other.has_value())
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
        if (other->has_value())
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
        if (other->has_value())
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
struct optional_move_assign_ : optional_copy_assign_<T>
{
    using optional_copy_assign_<T>::optional_copy_assign_;

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
struct optional_move_assign_<T, true> :optional_copy_assign_<T>
{
    using optional_copy_assign_<T>::optional_copy_assign_;
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

    static_assert(!std::is_same<in_place_t, typename std::decay<T>::type>::value, "instatiation with in_place_t is ill-formed");
    static_assert(!std::is_same<nullopt_t, typename std::decay<T>::type>::value, "instatiation with nullopt_t is ill-formed");

public:
    using value_type = T;

    constexpr optional() noexcept = default;
    constexpr optional(pd::nullopt_t) noexcept {}

    constexpr optional(const optional&) = default;
    constexpr optional(optional&&) = default;

    // In place construction
    template<typename... Args>
    constexpr explicit optional(detail::enable_if_t<std::is_constructible<T, Args...>::value, pd::in_place_t>,
            Args&&... args) : base(in_place, std::forward<Args>(args)...) {}

    template<typename I, typename... Args>
    constexpr explicit optional(
            detail::enable_if_t<std::is_constructible<T, std::initializer_list<I>&, Args...>::value,
            in_place_t>, std::initializer_list<I> ilist, Args&&... args)
    {
        this->construct(ilist, std::forward<Args>(args)...);
    }

    // Contruct stored value with value of type U
    template<typename U = T,
             detail::enable_if_t<std::is_convertible<U&&, T>::value> * = nullptr>
    constexpr optional(U &&u) : base(in_place, std::forward<U>(u)) {}

    template<typename U = T,
             detail::enable_if_t<!std::is_convertible<U&&, T>::value> * = nullptr>
    constexpr explicit optional(U &&u) : base(in_place, std::forward<U>(u)) {}

    // Copy constructor
    template<typename U = T,
            detail::enable_if_t<std::is_constructible<T, U&&>::value &&
                                std::is_convertible<const U&, T>::value> * = nullptr>
    constexpr optional(const optional<U> &other)
    {
        if (other.has_value())
            this->construct(other.get());
    }

    template<typename U = T,
            detail::enable_if_t<std::is_constructible<T, U&&>::value &&
                                !std::is_convertible<const U&, T>::value> * = nullptr>
    constexpr explicit optional(const optional<U> &other)
    {
        if (other.has_value())
            this->construct(other.get());
    }

    // Move constructor
    template<typename U = T,
            detail::enable_if_t<std::is_constructible<T, U&&>::value &&
                                std::is_convertible<U&&, T>::value> * = nullptr>
    constexpr optional(optional<U> &&other)
    {
        if (other.has_value())
            this->construct(std::move(other.get()));
    }

    template<typename U = T,
            detail::enable_if_t<std::is_constructible<T, U&&>::value &&
                                !std::is_convertible<U&&, T>::value> * = nullptr>
    constexpr explicit optional(optional<U> &&other)
    {
        if (other.has_value())
            this->construct(std::move(other.get()));
    }

    // Destructor
    ~optional() = default;


    constexpr optional& operator= (pd::nullopt_t) noexcept
    {
        if (this->has_value())
        {
            this->value_.~T();
            this->is_set_ = false;
        }
        return *this;
    }

    constexpr optional& operator= (const optional&) = default;
    constexpr optional& operator= (optional&&) = default;

    template<typename U, detail::enable_if_t<std::is_constructible<T, U&&>::value && std::is_assignable<T&, U>::value>* = nullptr>
    constexpr optional& operator= (U &&u)
    {
        if(this->has_value())
            this->value_ = std::forward<U>(u);
        else
            this->construct(std::forward<U>(u));
        return *this;
    }

    template<typename U, 
        detail::enable_if_t<std::is_constructible<T, const U&>::value && std::is_assignable<T, const U&>::value> * = nullptr>
    constexpr optional& operator= (const optional<U> &other)
    {
        if (other.has_value())
        {
            this->value_ = other.value_;
            this->is_set_ = true;
        }
        else
            this->hard_reset();
        return *this;
    }

    template<typename U, 
        detail::enable_if_t<std::is_constructible<T, U&&>::value && std::is_assignable<T, U&&>::value> * = nullptr>
    constexpr optional& operator= (optional<U> &&other)
    {
        if (other.has_value())
        {
            this->value_ = std::move(other.value_);
            this->is_set_ = true;
        }
        else
            this->hard_reset();
        return *this;
    }

    constexpr const T* operator->() const
    {
        return std::addressof(this->value_);
    }

    constexpr T* operator->()
    {
        return std::addressof(this->value_);
    }

    constexpr const T& operator*() const&
    {
        return this->value_;
    }

    constexpr T& operator*() &
    {
        return this->value_;
    }
    constexpr const T&& operator*() const&&
    {
        return std::move(this->value_);
    }
    constexpr T&& operator*() && 
    {
        return std::move(this->value_);
    }

    constexpr explicit operator bool() const noexcept
    {
        return this->is_set_;
    }

    constexpr bool has_value() const noexcept
    {
        return this->is_set_;
    }

    constexpr T& value() &
    {
        if (this->has_value())
            return this->value_;
        throw bad_optional_access();
    }

    constexpr const T& value() const &
    {
        if (this->has_value())
            return this->value_;
        throw bad_optional_access();
    }

    constexpr T&& value() &&
    {
        if (this->has_value())
            return std::move(this->value_);
        throw bad_optional_access();
    }

    constexpr const T&& value() const &&
    {
        if (this->has_value())
            return std::move(this->value_);
        throw bad_optional_access();
    }

    template<typename U>
    constexpr T value_or(U &&u) const
    {
        static_assert(std::is_copy_constructible<T>::value &&
                      std::is_move_constructible<T>::value &&
                      std::is_convertible<U&&, T>::value,
                      "T must be copy/move constructible and convertible from U\n");
        return this->has_value() ? **this : static_cast<T>(std::forward<U>(u));
    }

    void reset()
    {
        if (this->has_value())
        {
            this->value_.~T();
            this->is_set_ = false;
        }
    }

    template<typename... Args>
    T& emplace(Args&&... args)
    {
        static_assert(std::is_constructible<T, Args...>::value,
                "T must be constructible with Args\n");
        if(this->has_value())
        {
            this->value_.~T();
            this->is_set_ = false;
        }
        this->construct(std::forward<Args>(args)...);
        return **this;
    }

    template<typename U, typename... Args>
    T& emplace(std::initializer_list<U> ilist, Args&&... args)
    {
        static_assert(std::is_constructible<T, std::initializer_list<U>, Args...>::value,
                "T must be constructible with initializer_list<U> and Args\n");
        if(this->has_value())
        {
            this->value_.~T();
            this->is_set_ = false;
        }
        this->construct(ilist, std::forward<Args>(args)...);
        return **this;
    }
};

} // namespace pd

template<typename T, typename U>
inline constexpr bool operator==(const pd::optional<T> &lhs,
                                 const pd::optional<U> &rhs) {
      return lhs.has_value() == rhs.has_value() &&
             (!lhs.has_value() || *lhs == *rhs);
}

template<typename T, typename U>
inline constexpr bool operator!=(const pd::optional<T> &lhs,
                                 const pd::optional<U> &rhs) {
      return lhs.has_value() != rhs.has_value() ||
             (lhs.has_value() && *lhs != *rhs);
}

template<typename T, typename U>
inline constexpr bool operator<(const pd::optional<T> &lhs,
                                const pd::optional<U> &rhs) {
      return rhs.has_value() && (!lhs.has_value() || *lhs < *rhs);
}

template<typename T, typename U>
inline constexpr bool operator>(const pd::optional<T> &lhs,
                                const pd::optional<U> &rhs) {
      return lhs.has_value() && (!rhs.has_value() || *lhs > *rhs);
}

template<typename T, typename U>
inline constexpr bool operator<=(const pd::optional<T> &lhs,
                                 const pd::optional<U> &rhs) {
      return !lhs.has_value() || (rhs.has_value() && *lhs <= *rhs);
}

template<typename T, typename U>
inline constexpr bool operator>=(const pd::optional<T> &lhs,
                                 const pd::optional<U> &rhs) {
      return !rhs.has_value() || (lhs.has_value() && *lhs >= *rhs);
}

template<typename T>
inline constexpr bool operator==(const pd::optional<T> &lhs, pd::nullopt_t) noexcept {
      return !lhs.has_value();
}

template<typename T>
inline constexpr bool operator==(pd::nullopt_t, const pd::optional<T> &rhs) noexcept {
      return !rhs.has_value();
}

template<typename T>
inline constexpr bool operator!=(const pd::optional<T> &lhs, pd::nullopt_t) noexcept {
      return lhs.has_value();
}

template<typename T>
inline constexpr bool operator!=(pd::nullopt_t, const pd::optional<T> &rhs) noexcept {
      return rhs.has_value();
}

template<typename T>
inline constexpr bool operator<(const pd::optional<T> &, pd::nullopt_t) noexcept {
      return false;
}

template<typename T>
inline constexpr bool operator<(pd::nullopt_t, const pd::optional<T> &rhs) noexcept {
      return rhs.has_value();
}

template<typename T>
inline constexpr bool operator<=(const pd::optional<T> &lhs, pd::nullopt_t) noexcept {
      return !lhs.has_value();
}

template<typename T>
inline constexpr bool operator<=(pd::nullopt_t, const pd::optional<T> &) noexcept {
      return true;
}

template<typename T>
inline constexpr bool operator>(const pd::optional<T> &lhs, pd::nullopt_t) noexcept {
      return lhs.has_value();
}

template<typename T>
inline constexpr bool operator>(pd::nullopt_t, const pd::optional<T> &) noexcept {
      return false;
}

template<typename T>
inline constexpr bool operator>=(const pd::optional<T> &, pd::nullopt_t) noexcept {
      return true;
}

template<typename T>
inline constexpr bool operator>=(pd::nullopt_t, const pd::optional<T> &rhs) noexcept {
      return !rhs.has_value();
}

template<typename T, typename U>
inline constexpr bool operator==(const pd::optional<T> &lhs, const U &rhs) {
      return lhs.has_value() ? *lhs == rhs : false;
}

template<typename T, typename U>
inline constexpr bool operator==(const U &lhs, const pd::optional<T> &rhs) {
    return rhs.has_value() ? lhs == *rhs : false;
}

template<typename T, typename U>
inline constexpr bool operator!=(const pd::optional<T> &lhs, const U &rhs) {
      return lhs.has_value() ? *lhs != rhs : true;
}

template<typename T, typename U>
inline constexpr bool operator!=(const U &lhs, const pd::optional<T> &rhs) {
      return rhs.has_value() ? lhs != *rhs : true;
}

template<typename T, typename U>
inline constexpr bool operator<(const pd::optional<T> &lhs, const U &rhs) {
      return lhs.has_value() ? *lhs < rhs : true;
}

template<typename T, typename U>
inline constexpr bool operator<(const U &lhs, const pd::optional<T> &rhs) {
    return rhs.has_value() ? lhs < *rhs : false;
}

template<typename T, typename U>
inline constexpr bool operator<=(const pd::optional<T> &lhs, const U &rhs) {
      return lhs.has_value() ? *lhs <= rhs : true;
}

template<typename T, typename U>
inline constexpr bool operator<=(const U &lhs, const pd::optional<T> &rhs) {
      return rhs.has_value() ? lhs <= *rhs : false;
}

template<typename T, typename U>
inline constexpr bool operator>(const pd::optional<T> &lhs, const U &rhs) {
      return lhs.has_value() ? *lhs > rhs : false;
}

template<typename T, typename U>
inline constexpr bool operator>(const U &lhs, const pd::optional<T> &rhs) {
    return rhs.has_value() ? lhs > *rhs : true;
}

template<typename T, typename U>
inline constexpr bool operator>=(const pd::optional<T> &lhs, const U &rhs) {
      return lhs.has_value() ? *lhs >= rhs : false;
}

template<typename T, typename U>
inline constexpr bool operator>=(const U &lhs, const pd::optional<T> &rhs) {
      return rhs.has_value() ? lhs >= *rhs : true;
}

template<typename T,
          pd::detail::enable_if_t<std::is_move_constructible<T>::value> * = nullptr,
          pd::detail::enable_if_t<std::is_swappable<T>::value> * = nullptr>
void swap(pd::optional<T> &lhs, pd::optional<T> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
    return lhs.swap(rhs);
}

namespace pd
{
template<typename T>
constexpr pd::optional<std::decay_t<T>> make_optional(T &&t)
{
    return pd::optional<T>(std::forward<T>(t));
}

template<typename T, typename... Args>
constexpr pd::optional<std::decay_t<T>> make_optional(Args&&... args)
{
    return pd::optional<T>(pd::in_place, std::forward<T>(args)...);
}

template<typename T, typename U, typename... Args>
constexpr pd::optional<std::decay_t<T>> make_optional(std::initializer_list<U> ilist, Args&&... args)
{
    return pd::optional<T>(pd::in_place, ilist, std::forward<T>(args)...);
}
} // namespace pd

#endif // PD_OPTIONAL_OPTIONAL_HH_
