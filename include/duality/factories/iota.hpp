// This file is part of https://github.com/btzy/duality
#pragma once

// A view that iotas, either finitely or infinitely.

#include <concepts>
#include <iterator>
#include <utility>

#include <duality/core_view.hpp>

namespace duality {

template <typename T, typename Bound>
    requires std::incrementable<std::remove_cvref_t<T>>
class iota_view;
template <typename T, typename Index>
    requires std::incrementable<std::remove_cvref_t<T>> &&
             (std::integral<Index> || std::same_as<Index, no_index_type_t>)
class infinite_iota_view;

namespace impl {

template <typename T>
concept decrementable = std::regular<T> && requires(T& t) {
    typename std::iter_difference_t<T>;
    requires std::integral<std::iter_difference_t<T>>;
    { --t } -> std::same_as<T&>;
    { t-- } -> std::same_as<T>;
};

template <typename T>
concept advanceable = std::incrementable<T> && decrementable<T> &&
                      requires(T& t, T& t2, const typename std::iter_difference_t<T>& d) {
                          { t += d } -> std::same_as<T&>;
                          { t -= d } -> std::same_as<T&>;
                          { t + d } -> std::same_as<T>;
                          { t - d } -> std::same_as<T>;
                          { t - t2 } -> std::integral;
                      };

template <std::incrementable T, typename Index>
    requires std::integral<Index> || std::same_as<Index, no_index_type_t>
class iota_forward_iterator;

template <std::incrementable T, typename Bound, typename Index>
    requires std::integral<Index> || std::same_as<Index, no_index_type_t>
class iota_backward_iterator;

template <std::incrementable T, typename Index>
    requires std::integral<Index> || std::same_as<Index, no_index_type_t>
class infinite_nonreversible_iota_forward_iterator;

class iota_sentinel {};

template <std::incrementable T, typename Index>
    requires std::integral<Index> || std::same_as<Index, no_index_type_t>
class iota_forward_iterator {
   private:
    using element_type = T;
    template <typename T2, typename>
        requires std::incrementable<std::remove_cvref_t<T2>>
    friend class duality::iota_view;
    template <typename T2, typename Index2>
        requires std::incrementable<std::remove_cvref_t<T2>> &&
                 (std::integral<Index2> || std::same_as<Index2, no_index_type_t>)
    friend class duality::infinite_iota_view;
    template <std::incrementable, typename, typename Index2>
        requires std::integral<Index2> || std::same_as<Index2, no_index_type_t>
    friend class iota_backward_iterator;
    [[no_unique_address]] T value_;
    constexpr iota_forward_iterator(wrapping_construct_t, const T& value) noexcept
        : value_(value) {}

   public:
    using index_type = Index;
    constexpr element_type next() noexcept { return value_++; }
    template <typename Bound>
    constexpr optional<element_type> next(
        const iota_backward_iterator<T, Bound, Index>& end_i) noexcept {
        if (value_ == end_i.value_) return nullopt;
        return value_++;
    }
    constexpr optional<element_type> next(const iota_sentinel&) noexcept { return value_++; }
    constexpr void skip() noexcept { ++value_; }
    template <typename Bound>
    constexpr bool skip(const iota_backward_iterator<T, Bound, Index>& end_i) noexcept {
        if (value_ == end_i.value_) return false;
        return ++value_;
        return true;
    }
    constexpr bool skip(const iota_sentinel&) noexcept {
        ++value_;
        return true;
    }
    constexpr void skip(Index index) noexcept
        requires std::integral<Index>
    {
        value_ += index;
    }
    template <typename Bound>
    constexpr Index skip(Index index, const iota_backward_iterator<T, Bound, Index>& end_i) noexcept
        requires std::integral<Index>
    {
        Index diff = static_cast<Index>(end_i.value_ - value_);
        if (static_cast<Index>(end_i.value_ - value_) >= index) {
            value_ += index;
            return index;
        }
        value_ = end_i.value_;
        return diff;
    }
    constexpr Index skip(Index index, const iota_sentinel&) noexcept
        requires std::integral<Index>
    {
        value_ += index;
        return index;
    }
    constexpr decltype(auto) invert() const noexcept {
        return iota_backward_iterator<T, T, Index>(wrapping_construct, value_);
    }
};

template <std::incrementable T, typename Bound, typename Index>
    requires std::integral<Index> || std::same_as<Index, no_index_type_t>
class iota_backward_iterator {
   private:
    using element_type = T;
    template <typename T2, typename>
        requires std::incrementable<std::remove_cvref_t<T2>>
    friend class duality::iota_view;
    template <std::incrementable, typename Index2>
        requires std::integral<Index2> || std::same_as<Index2, no_index_type_t>
    friend class iota_forward_iterator;
    [[no_unique_address]] Bound value_;
    constexpr iota_backward_iterator(wrapping_construct_t, const T& value) noexcept
        : value_(value) {}

   public:
    using index_type = Index;
    constexpr element_type next() noexcept
        requires decrementable<T>
    {
        return --value_;
    }
    constexpr optional<element_type> next(const iota_forward_iterator<T, Index>& end_i) noexcept
        requires decrementable<T>
    {
        if (value_ == end_i.value_) return nullopt;
        return --value_;
    }
    constexpr void skip() noexcept
        requires decrementable<T>
    {
        --value_;
    }
    constexpr bool skip(const iota_forward_iterator<T, Index>& end_i) noexcept
        requires decrementable<T>
    {
        if (value_ == end_i.value_) return false;
        --value_;
        return true;
    }
    constexpr void skip(Index index) noexcept
        requires std::integral<Index>
    {
        value_ -= index;
    }
    constexpr Index skip(Index index, const iota_forward_iterator<T, Index>& end_i) noexcept
        requires std::integral<Index>
    {
        Index diff = static_cast<Bound>(value_ - end_i.value_);
        if (diff >= index) {
            value_ -= index;
            return index;
        }
        value_ = end_i.value_;
        return diff;
    }
    constexpr decltype(auto) invert() const noexcept
        requires decrementable<T>
    {
        return iota_forward_iterator<T, Index>(wrapping_construct, value_);
    }
};

}  // namespace impl

template <typename T, typename Bound>
    requires std::incrementable<std::remove_cvref_t<T>>
class iota_view {
   private:
    [[no_unique_address]] T value_;
    [[no_unique_address]] Bound bound_;

   public:
    using index_type = std::conditional_t<impl::advanceable<T>,
                                          std::make_unsigned_t<decltype(bound_ - value_)>,
                                          no_index_type_t>;
    template <typename T2, typename Bound2>
    constexpr iota_view(wrapping_construct_t, T2&& value, Bound2&& bound) noexcept(
        std::is_nothrow_constructible_v<T, T2> && std::is_nothrow_constructible_v<Bound, Bound2>)
        : value_(std::forward<T2>(value)), bound_(std::forward<Bound2>(bound)) {}
    constexpr auto forward_iter() const noexcept {
        return impl::iota_forward_iterator<std::remove_cvref_t<T>, index_type>(wrapping_construct,
                                                                               value_);
    }
    constexpr auto backward_iter() const noexcept {
        return impl::iota_backward_iterator<std::remove_cvref_t<T>,
                                            std::remove_cvref_t<Bound>,
                                            index_type>(wrapping_construct, bound_);
    }
    constexpr bool empty() const noexcept
        requires requires { value_ == bound_; }
    {
        return value_ == bound_;
    }
    constexpr index_type size() const noexcept
        requires requires { bound_ - value_; }
    {
        return bound_ - value_;
    }
    constexpr T operator[](index_type index) const noexcept
        requires impl::advanceable<std::remove_cvref_t<T>>
    {
        return value_ + index;
    }
};

template <typename T, typename Index>
    requires std::incrementable<std::remove_cvref_t<T>> &&
             (std::integral<Index> || std::same_as<Index, no_index_type_t>)
class infinite_iota_view {
   private:
    [[no_unique_address]] T value_;

   public:
    using index_type = Index;
    template <typename T2>
    constexpr infinite_iota_view(wrapping_construct_t,
                                 T2&& value) noexcept(std::is_nothrow_constructible_v<T, T2>)
        : value_(std::forward<T2>(value)) {}
    constexpr auto forward_iter() const noexcept {
        return impl::iota_forward_iterator<std::remove_cvref_t<T>, index_type>(wrapping_construct,
                                                                               value_);
    }
    constexpr auto backward_iter() const noexcept { return impl::iota_sentinel{}; }
    constexpr static bool empty() noexcept { return false; }
    constexpr static infinite_t size() noexcept { return infinite_t{}; }
    constexpr T operator[](index_type index) const noexcept
        requires impl::advanceable<std::remove_cvref_t<T>>
    {
        return value_ + index;
    }
};

template <typename T2, typename Bound2>
iota_view(wrapping_construct_t, T2&& value, Bound2&& bound) -> iota_view<T2, Bound2>;

template <typename T2>
infinite_iota_view(wrapping_construct_t, T2&& value)
    -> infinite_iota_view<T2,
                          std::conditional_t<std::integral<std::iter_difference_t<T2>>,
                                             std::make_unsigned_t<std::iter_difference_t<T2>>,
                                             no_index_type_t>>;

namespace impl {
struct iota {
    template <typename T, typename Bound>
    constexpr DUALITY_STATIC_CALL auto operator()(T&& t, Bound&& bound) DUALITY_CONST_CALL {
        return iota_view(wrapping_construct, std::forward<T>(t), std::forward<Bound>(bound));
    }
    template <typename T>
    constexpr DUALITY_STATIC_CALL auto operator()(T&& t) DUALITY_CONST_CALL {
        return infinite_iota_view(wrapping_construct, std::forward<T>(t));
    }
};
}  // namespace impl

namespace factories {
constexpr inline impl::iota iota;
}

}  // namespace duality
