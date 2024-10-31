// This file is part of https://github.com/btzy/duality
#pragma once

#include <concepts>
#include <optional>

namespace duality {
using nullopt_t = std::nullopt_t;
constexpr inline nullopt_t nullopt = std::nullopt;

/// An optional type that also supports references.  Ideally, this type shouldn't be movable, but
/// due to C++ limitations it has to be.
template <typename T>
class optional {
   private:
    std::optional<T> opt_;

   public:
    using value_type = T;

    constexpr optional() : opt_() {}
    constexpr optional(nullopt_t) : opt_() {}
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr optional(Args&&... args) : opt_(std::forward<Args>(args)...) {}
    constexpr optional(optional&& other) = default;
    constexpr optional(const optional& other) = default;
    constexpr optional& operator=(optional&& other) = delete;
    constexpr optional& operator=(const optional& other) = delete;

    constexpr explicit operator bool() const noexcept { return opt_.has_value(); }
    constexpr const T* operator->() const noexcept { return &*opt_; }
    constexpr T* operator->() noexcept { return &*opt_; }
    constexpr const T& operator*() const& noexcept { return *opt_; }
    constexpr T& operator*() & noexcept { return *opt_; }
    constexpr const T&& operator*() const&& noexcept { return *std::move(opt_); }
    constexpr T&& operator*() && noexcept { return *std::move(opt_); }
};

template <typename T>
class optional<T&> {
   private:
    T* opt_;

   public:
    using value_type = T&;

    constexpr optional() : opt_(nullptr) {}
    constexpr optional(nullopt_t) : opt_(nullptr) {}
    constexpr optional(T& ref) : opt_(&ref) {}
    constexpr optional(optional&& other) = default;
    constexpr optional(const optional& other) = default;
    constexpr optional& operator=(optional&& other) = delete;
    constexpr optional& operator=(const optional& other) = delete;

    constexpr explicit operator bool() const noexcept { return opt_; }
    constexpr T* operator->() const noexcept { return opt_; }
    constexpr T& operator*() const noexcept { return *opt_; }
};

template <typename T>
class optional<T&&> {
   private:
    T* opt_;

   public:
    using value_type = T&&;

    constexpr optional() : opt_(nullptr) {}
    constexpr optional(nullopt_t) : opt_(nullptr) {}
    constexpr optional(T&& ref) : opt_(&ref) {}
    constexpr optional(optional&& other) = default;
    constexpr optional(const optional& other) = default;
    constexpr optional& operator=(optional&& other) = delete;
    constexpr optional& operator=(const optional& other) = delete;

    constexpr explicit operator bool() const noexcept { return opt_; }
    constexpr T* operator->() const noexcept { return opt_; }
    constexpr T& operator*() const& noexcept { return std::move(*opt_); }
    constexpr T&& operator*() const&& noexcept { return std::move(*opt_); }
};

// Type trait to detect if T is a optional.
template <typename T>
struct is_optional : std::false_type {};
template <typename T>
struct is_optional<optional<T>> : std::true_type {};

template <typename T>
concept option = is_optional<T>::value;

}  // namespace duality
