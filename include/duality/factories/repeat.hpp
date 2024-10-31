// This file is part of https://github.com/btzy/duality
#pragma once

// A view that repeats, either finitely or infinitely.

#include <concepts>
#include <utility>

#include <duality/core_view.hpp>

namespace duality {

template <typename T, std::integral Count>
class repeat_view;
template <typename T, std::integral Index>
class infinite_repeat_view;

namespace impl {

template <typename T, std::integral Count>
class repeat_forward_iterator;
template <typename T, std::integral Count>
class repeat_backward_iterator;

class repeat_sentinel {};

template <typename T, std::integral Count>
class repeat_forward_iterator {
   private:
    using element_type = const T&;
    template <typename, std::integral>
    friend class duality::repeat_view;
    template <typename, std::integral>
    friend class duality::infinite_repeat_view;
    friend class repeat_backward_iterator<T, Count>;
    const T* value_;
    [[no_unique_address]] Count index_;
    constexpr repeat_forward_iterator(wrapping_construct_t, const T& value, Count index) noexcept
        : value_(&value), index_(index) {}

   public:
    using index_type = Count;
    constexpr element_type next() noexcept {
        ++index_;
        return *value_;
    }
    constexpr optional<element_type> next(
        const repeat_backward_iterator<T, Count>& end_i) noexcept {
        if (index_ == end_i.index_) return nullopt;
        ++index_;
        return *value_;
    }
    constexpr optional<element_type> next(const repeat_sentinel&) noexcept {
        ++index_;
        return *value_;
    }
    constexpr void skip() noexcept { ++index_; }
    constexpr bool skip(const repeat_backward_iterator<T, Count>& end_i) noexcept {
        if (index_ == end_i.index_) return false;
        ++index_;
        return true;
    }
    constexpr bool skip(const repeat_sentinel&) noexcept {
        ++index_;
        return true;
    }
    constexpr void skip(Count count) noexcept { index_ += count; }
    constexpr Count skip(Count count, const repeat_backward_iterator<T, Count>& end_i) noexcept {
        Count diff = static_cast<Count>(end_i.index_ - index_);
        if (diff >= count) {
            index_ += count;
            return count;
        }
        index_ = end_i.index_;
        return diff;
    }
    constexpr Count skip(Count count, const repeat_sentinel&) noexcept {
        index_ += count;
        return count;
    }
    constexpr decltype(auto) invert() const noexcept {
        return repeat_backward_iterator<T, Count>(wrapping_construct, *value_, index_);
    }
};

template <typename T, std::integral Count>
class repeat_backward_iterator {
   private:
    using element_type = const T&;
    template <typename, std::integral>
    friend class duality::repeat_view;
    friend class repeat_forward_iterator<T, Count>;
    const T* value_;
    [[no_unique_address]] Count index_;
    constexpr repeat_backward_iterator(wrapping_construct_t, const T& value, Count index) noexcept
        : value_(&value), index_(index) {}

   public:
    using index_type = Count;
    constexpr element_type next() noexcept {
        --index_;
        return *value_;
    }
    constexpr optional<element_type> next(const repeat_forward_iterator<T, Count>& end_i) noexcept {
        if (index_ == end_i.index_) return nullopt;
        --index_;
        return *value_;
    }
    constexpr void skip() noexcept { --index_; }
    constexpr bool skip(const repeat_forward_iterator<T, Count>& end_i) noexcept {
        if (index_ == end_i.index_) return false;
        --index_;
        return true;
    }
    constexpr void skip(Count count) noexcept { index_ -= count; }
    constexpr Count skip(Count count, const repeat_forward_iterator<T, Count>& end_i) noexcept {
        Count diff = static_cast<Count>(index_ - end_i.index_);
        if (diff >= count) {
            index_ -= count;
            return count;
        }
        index_ = end_i.index_;
        return diff;
    }
    constexpr decltype(auto) invert() const noexcept {
        return repeat_forward_iterator<T, Count>(wrapping_construct, *value_, index_);
    }
};

}  // namespace impl

template <typename T, std::integral Count>
class repeat_view {
   private:
    [[no_unique_address]] T value_;
    [[no_unique_address]] Count count_;

   public:
    using index_type = Count;
    template <typename T2>
    constexpr repeat_view(wrapping_construct_t,
                          T2&& value,
                          Count count) noexcept(std::is_nothrow_constructible_v<T, T2>)
        : value_(std::forward<T2>(value)), count_(count) {}
    constexpr auto forward_iter() const noexcept {
        return impl::repeat_forward_iterator<std::remove_cvref_t<T>, Count>(
            wrapping_construct, value_, 0);
    }
    constexpr auto backward_iter() const noexcept {
        return impl::repeat_backward_iterator<std::remove_cvref_t<T>, Count>(
            wrapping_construct, value_, count_);
    }
    constexpr bool empty() const noexcept { return count_ == 0; }
    constexpr Count size() const noexcept { return count_; }
    constexpr const std::remove_cvref_t<T>& operator[](Count) const noexcept { return value_; }
};

template <typename T, std::integral Index>
class infinite_repeat_view {
   private:
    [[no_unique_address]] T value_;

   public:
    using index_type = Index;
    template <typename T2>
    constexpr infinite_repeat_view(wrapping_construct_t,
                                   T2&& value) noexcept(std::is_nothrow_constructible_v<T, T2>)
        : value_(std::forward<T2>(value)) {}
    constexpr auto forward_iter() const noexcept {
        return impl::repeat_forward_iterator<std::remove_cvref_t<T>, index_type>(
            wrapping_construct, value_, 0);
    }
    constexpr auto backward_iter() const noexcept { return impl::repeat_sentinel{}; }
    constexpr static bool empty() noexcept { return false; }
    constexpr static infinite_t size() noexcept { return infinite_t{}; }
    constexpr const T& operator[](index_type) const noexcept { return value_; }
};

template <typename T2, std::integral Count>
repeat_view(wrapping_construct_t, T2&& value, Count count) -> repeat_view<T2, Count>;

template <typename T2>
infinite_repeat_view(wrapping_construct_t, T2&& value) -> infinite_repeat_view<T2, size_t>;

namespace impl {
struct repeat {
    template <typename T, std::integral Count>
    constexpr DUALITY_STATIC_CALL auto operator()(T&& t, Count count) DUALITY_CONST_CALL {
        return repeat_view(wrapping_construct, std::forward<T>(t), count);
    }
    template <typename T>
    constexpr DUALITY_STATIC_CALL auto operator()(T&& t) DUALITY_CONST_CALL {
        return infinite_repeat_view(wrapping_construct, std::forward<T>(t));
    }
};
}  // namespace impl

namespace factories {
constexpr inline impl::repeat repeat;
}

}  // namespace duality
