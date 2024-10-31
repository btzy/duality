// This file is part of https://github.com/btzy/duality
#pragma once

// A view that contains a single element.

#include <concepts>
#include <utility>

#include <duality/core_view.hpp>

namespace duality {

template <typename T>
class single_view;

namespace impl {

template <typename T>
class single_forward_iterator;
template <typename T>
class single_backward_iterator;

template <typename T>
class single_forward_iterator {
   private:
    using element_type = const T&;
    template <typename T2>
    friend class duality::single_view;
    friend class single_backward_iterator<T>;
    const T* value_;
    constexpr single_forward_iterator(wrapping_construct_t, const T* value) noexcept
        : value_(value) {}

   public:
    using index_type = size_t;
    constexpr element_type next() noexcept { return *value_++; }
    constexpr optional<element_type> next(const single_backward_iterator<T>& end_i) noexcept {
        if (value_ == end_i.value_) return nullopt;
        return *value_++;
    }
    constexpr void skip() noexcept { ++value_; }
    constexpr bool skip(const single_backward_iterator<T>& end_i) noexcept {
        if (value_ == end_i.value_) return false;
        ++value_;
        return true;
    }
    constexpr void skip(size_t count) noexcept { value_ += count; }
    constexpr size_t skip(size_t count, const single_backward_iterator<T>& end_i) noexcept {
        size_t diff = static_cast<size_t>(end_i.value_ - value_);
        if (diff >= count) {
            value_ += count;
            return count;
        }
        value_ = end_i.value_;
        return diff;
    }
    constexpr decltype(auto) invert() const noexcept {
        return single_backward_iterator<T>(wrapping_construct, value_);
    }
};

template <typename T>
class single_backward_iterator {
   private:
    using element_type = const T&;
    template <typename T2>
    friend class duality::single_view;
    friend class single_forward_iterator<T>;
    const T* value_;
    constexpr single_backward_iterator(wrapping_construct_t, const T* value) noexcept
        : value_(value) {}

   public:
    using index_type = size_t;
    constexpr element_type next() noexcept { return *--value_; }
    constexpr optional<element_type> next(const single_forward_iterator<T>& end_i) noexcept {
        if (value_ == end_i.value_) return nullopt;
        return *--value_;
    }
    constexpr void skip() noexcept { --value_; }
    constexpr bool skip(const single_forward_iterator<T>& end_i) noexcept {
        if (value_ == end_i.value_) return false;
        --value_;
        return true;
    }
    constexpr void skip(size_t count) noexcept { value_ -= count; }
    constexpr size_t skip(size_t count, const single_forward_iterator<T>& end_i) noexcept {
        size_t diff = static_cast<size_t>(value_ - end_i.value_);
        if (diff >= count) {
            value_ -= count;
            return count;
        }
        value_ = end_i.value_;
        return diff;
    }
    constexpr decltype(auto) invert() const noexcept {
        return single_forward_iterator<T>(wrapping_construct, value_);
    }
};

}  // namespace impl

template <typename T>
class single_view {
   private:
    [[no_unique_address]] T value_;
    using base_type = std::remove_cvref_t<T>;

   public:
    using index_type = size_t;
    template <typename T2>
    constexpr single_view(wrapping_construct_t,
                          T2&& value) noexcept(std::is_nothrow_constructible_v<T, T2>)
        : value_(std::forward<T2>(value)) {}
    constexpr auto forward_iter() const noexcept {
        return impl::single_forward_iterator<base_type>(wrapping_construct, &value_);
    }
    constexpr auto backward_iter() const noexcept {
        return impl::single_backward_iterator<base_type>(wrapping_construct, &value_ + 1);
    }
    constexpr static bool empty() noexcept { return false; }
    constexpr static size_t size() noexcept { return 1; }
    constexpr const base_type& operator[](index_type) const noexcept { return value_; }
};

template <typename T2>
single_view(wrapping_construct_t, T2&& value) -> single_view<T2>;

namespace impl {
struct single {
    template <typename T>
    constexpr DUALITY_STATIC_CALL auto operator()(T&& t) DUALITY_CONST_CALL {
        return single_view(wrapping_construct, std::forward<T>(t));
    }
};
}  // namespace impl

namespace factories {
constexpr inline impl::single single;
}

}  // namespace duality
