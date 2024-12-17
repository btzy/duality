// This file is part of https://github.com/btzy/duality
#pragma once

// A view that iotas, either finitely or infinitely.

#include <concepts>
#include <iterator>
#include <utility>

#include <duality/core_view.hpp>

namespace duality {

namespace impl {

template <typename T>
concept incrementable = std::incrementable<T>;

template <typename T>
concept decrementable = std::regular<T> && requires(T& t) {
    typename std::iter_difference_t<T>;
    requires std::integral<std::iter_difference_t<T>>;
    { --t } -> std::same_as<T&>;
    { t-- } -> std::same_as<T>;
};

template <typename TBegin, typename TEnd>
concept iota_common_or_one_way =
    (impl::incrementable<std::remove_cvref_t<TBegin>> ||
     impl::decrementable<std::remove_cvref_t<
         TEnd>>)&&(!impl::incrementable<std::remove_cvref_t<TBegin>> ||
                   !impl::decrementable<std::remove_cvref_t<TEnd>> ||
                   std::same_as<std::remove_cvref_t<TBegin>, std::remove_cvref_t<TEnd>>);

}  // namespace impl

template <typename TBegin, typename TEnd>
    requires impl::iota_common_or_one_way<TBegin, TEnd>
class iota_view;
template <typename TBegin>
    requires impl::incrementable<std::remove_cvref_t<TBegin>>
class infinite_iota_view;

namespace impl {

template <typename T>
concept advanceable = incrementable<T> && decrementable<T> &&
                      requires(T& t, T& t2, const typename std::iter_difference_t<T>& d) {
                          { t += d } -> std::same_as<T&>;
                          { t -= d } -> std::same_as<T&>;
                          { t + d } -> std::same_as<T>;
                          { t - d } -> std::same_as<T>;
                          { t - t2 } -> std::integral;
                      };

template <typename T, typename Index>
    requires std::integral<Index> || std::same_as<Index, no_index_type_t>
class iota_forward_iterator;

template <typename T, typename Index>
    requires std::integral<Index> || std::same_as<Index, no_index_type_t>
class iota_backward_iterator;

class iota_sentinel {};

template <typename T, typename Index>
    requires std::integral<Index> || std::same_as<Index, no_index_type_t>
class iota_forward_iterator {
   private:
    using element_type = T;
    template <typename TBegin, typename TEnd>
        requires impl::iota_common_or_one_way<TBegin, TEnd>
    friend class duality::iota_view;
    template <typename, typename Index2>
        requires std::integral<Index2> || std::same_as<Index2, no_index_type_t>
    friend class iota_backward_iterator;
    [[no_unique_address]] const T& value_;
    constexpr iota_forward_iterator(wrapping_construct_t, const T& value) noexcept
        : value_(value) {}
};

template <impl::incrementable T, typename Index>
    requires std::integral<Index> || std::same_as<Index, no_index_type_t>
class iota_forward_iterator<T, Index> {
   private:
    using element_type = T;
    template <typename TBegin, typename TEnd>
        requires impl::iota_common_or_one_way<TBegin, TEnd>
    friend class duality::iota_view;
    template <typename TBegin>
        requires impl::incrementable<std::remove_cvref_t<TBegin>>
    friend class duality::infinite_iota_view;
    template <typename, typename Index2>
        requires std::integral<Index2> || std::same_as<Index2, no_index_type_t>
    friend class iota_backward_iterator;
    [[no_unique_address]] T value_;
    constexpr iota_forward_iterator(wrapping_construct_t, const T& value) noexcept
        : value_(value) {}

   public:
    using index_type = Index;
    constexpr element_type next() noexcept { return value_++; }
    template <std::equality_comparable_with<T> T2>
    constexpr optional<element_type> next(const iota_backward_iterator<T2, Index>& end_i) noexcept {
        if (value_ == end_i.value_) return nullopt;
        return value_++;
    }
    constexpr optional<element_type> next(const iota_sentinel&) noexcept { return value_++; }
    constexpr void skip() noexcept { ++value_; }

    template <std::equality_comparable_with<T> T2>
    constexpr bool skip(const iota_backward_iterator<T2, Index>& end_i) noexcept {
        if (value_ == end_i.value_) return false;
        ++value_;
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
    template <std::equality_comparable_with<T> T2>
    constexpr Index skip(Index index, const iota_backward_iterator<T2, Index>& end_i) noexcept
        requires std::integral<Index>
    {
        Index diff = static_cast<Index>(end_i.value_ - value_);
        if (diff >= index) {
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
        return iota_backward_iterator<T, Index>(wrapping_construct, value_);
    }
};

template <typename T, typename Index>
    requires std::integral<Index> || std::same_as<Index, no_index_type_t>
class iota_backward_iterator {
   private:
    using element_type = T;
    template <typename TBegin, typename TEnd>
        requires impl::iota_common_or_one_way<TBegin, TEnd>
    friend class duality::iota_view;
    template <typename, typename Index2>
        requires std::integral<Index2> || std::same_as<Index2, no_index_type_t>
    friend class iota_forward_iterator;
    [[no_unique_address]] const T& value_;
    constexpr iota_backward_iterator(wrapping_construct_t, const T& value) noexcept
        : value_(value) {}
};

template <impl::decrementable T, typename Index>
    requires std::integral<Index> || std::same_as<Index, no_index_type_t>
class iota_backward_iterator<T, Index> {
   private:
    using element_type = T;
    template <typename TBegin, typename TEnd>
        requires impl::iota_common_or_one_way<TBegin, TEnd>
    friend class duality::iota_view;
    template <typename, typename Index2>
        requires std::integral<Index2> || std::same_as<Index2, no_index_type_t>
    friend class iota_forward_iterator;
    [[no_unique_address]] T value_;
    constexpr iota_backward_iterator(wrapping_construct_t, const T& value) noexcept
        : value_(value) {}

   public:
    using index_type = Index;
    constexpr element_type next() noexcept { return --value_; }
    template <std::equality_comparable_with<T> T2>
    constexpr optional<element_type> next(const iota_forward_iterator<T2, Index>& end_i) noexcept {
        if (value_ == end_i.value_) return nullopt;
        return --value_;
    }
    constexpr void skip() noexcept { --value_; }
    template <std::equality_comparable_with<T> T2>
    constexpr bool skip(const iota_forward_iterator<T2, Index>& end_i) noexcept {
        if (value_ == end_i.value_) return false;
        --value_;
        return true;
    }
    constexpr void skip(Index index) noexcept
        requires std::integral<Index>
    {
        value_ -= index;
    }
    template <std::equality_comparable_with<T> T2>
    constexpr Index skip(Index index, const iota_forward_iterator<T2, Index>& end_i) noexcept
        requires std::integral<Index>
    {
        Index diff = static_cast<Index>(value_ - end_i.value_);
        if (diff >= index) {
            value_ -= index;
            return index;
        }
        value_ = end_i.value_;
        return diff;
    }
    constexpr decltype(auto) invert() const noexcept {
        return iota_forward_iterator<T, Index>(wrapping_construct, value_);
    }
};

template <typename...>
struct iota_index_type;
template <>
struct iota_index_type<> {
    using type = no_index_type_t;
};
template <typename T, typename... Ts>
struct iota_index_type<T, Ts...> {
    using type = typename iota_index_type<Ts...>::type;
};
template <advanceable T, typename... Ts>
struct iota_index_type<T, Ts...> {
    using type = decltype(std::declval<T>() - std::declval<T>());
};
template <typename... Ts>
using iota_index_type_t = typename iota_index_type<Ts...>::type;

}  // namespace impl

template <typename TBegin, typename TEnd>
    requires impl::iota_common_or_one_way<TBegin, TEnd>
class iota_view {
   private:
    [[no_unique_address]] TBegin begin_;
    [[no_unique_address]] TEnd end_;
    using index_type = impl::iota_index_type_t<TBegin, TEnd>;

   public:
    template <typename TBegin2, typename TEnd2>
    constexpr iota_view(wrapping_construct_t,
                        TBegin2&& begin,
                        TEnd2&& end) noexcept(std::is_nothrow_constructible_v<TBegin, TBegin2> &&
                                              std::is_nothrow_constructible_v<TEnd, TEnd2>)
        : begin_(std::forward<TBegin2>(begin)), end_(std::forward<TEnd2>(end)) {}
    constexpr auto forward_iter() const noexcept {
        return impl::iota_forward_iterator<std::remove_cvref_t<TBegin>, index_type>(
            wrapping_construct, begin_);
    }
    constexpr auto backward_iter() const noexcept {
        return impl::iota_backward_iterator<std::remove_cvref_t<TEnd>, index_type>(
            wrapping_construct, end_);
    }
    constexpr bool empty() const noexcept
        requires requires { begin_ == end_; }
    {
        return begin_ == end_;
    }
    constexpr index_type size() const noexcept
        requires requires { end_ - begin_; }
    {
        return end_ - begin_;
    }
};

template <typename TBegin>
    requires impl::incrementable<std::remove_cvref_t<TBegin>>
class infinite_iota_view {
   private:
    [[no_unique_address]] TBegin value_;
    using index_type = impl::iota_index_type_t<TBegin>;

   public:
    template <typename TBegin2>
    constexpr infinite_iota_view(wrapping_construct_t, TBegin2&& value) noexcept(
        std::is_nothrow_constructible_v<TBegin, TBegin2>)
        : value_(std::forward<TBegin2>(value)) {}
    constexpr auto forward_iter() const noexcept {
        return impl::iota_forward_iterator<std::remove_cvref_t<TBegin>, index_type>(
            wrapping_construct, value_);
    }
    constexpr auto backward_iter() const noexcept { return impl::iota_sentinel{}; }
    constexpr static bool empty() noexcept { return false; }
    constexpr static infinite_t size() noexcept { return infinite_t{}; }
};

template <typename TBegin, typename TEnd>
iota_view(wrapping_construct_t, TBegin&& begin, TEnd&& end) -> iota_view<TBegin, TEnd>;
template <typename TBegin>
infinite_iota_view(wrapping_construct_t, TBegin&& begin) -> infinite_iota_view<TBegin>;

namespace impl {
struct iota {
    template <typename TBegin, typename TEnd>
    constexpr DUALITY_STATIC_CALL auto operator()(TBegin&& begin, TEnd&& end) DUALITY_CONST_CALL {
        static_assert(iota_common_or_one_way<TBegin, TEnd>,
                      "Either begin and end are the same type after removing cvref, or one of them "
                      "is not iterable.");
        return iota_view(wrapping_construct, std::forward<TBegin>(begin), std::forward<TEnd>(end));
    }
    template <typename TBegin>
    constexpr DUALITY_STATIC_CALL auto operator()(TBegin&& begin) DUALITY_CONST_CALL {
        return infinite_iota_view(wrapping_construct, std::forward<TBegin>(begin));
    }
};
}  // namespace impl

namespace factories {
constexpr inline impl::iota iota;
}

}  // namespace duality
