// This file is part of https://github.com/btzy/duality
#pragma once

#include <algorithm>
#include <concepts>
#include <utility>

#include <duality/builtin_assume.hpp>
#include <duality/core_view.hpp>

/// Implementation for views::take.  Truncates the range after the given number of elements.
///
/// E.g.:
/// take({1, 3, 5, 7, 9}, 3) -> {1, 3, 5}
///
/// views::take accepts only forward_views.  The resultant view satisfies forward_view, but not
/// backward_view, even if the given view satisfies backward_view.  The resultant view also satifies
/// multipass_forward_view if the given view satisfies multipass_forward_view.
///
/// This view differs from eager_take in what backward_iter() does.  take does not compute the
/// appropriate position of the underlying backward iterator, and instead returns a sentinel.  The
/// end position can only be determined by advancing the forward iterator by the desired number of
/// positions.  As such, the resultant view is never a backward_view, even if the given view is a
/// backward_view.
///
/// However, the forward iterator of the resultant view satisfies all iterator concepts that the
/// forward iterator of the given view satisfies.  This makes it possible to get back a stronger
/// view type by passing the resultant view through some other adaptors, e.g. eager_chunk_view.

namespace duality {

template <forward_view V, std::integral Amount>
class take_view;

namespace impl {
template <iterator I, std::integral Amount>
class take_forward_iterator;
template <typename I, std::integral Amount>
class take_backward_iterator;
template <typename I>
class take_sentinel;

template <iterator I, std::integral Amount>
class take_forward_iterator {
   private:
    using element_type = iterator_element_type_t<I>;
    template <forward_view, std::integral>
    friend class duality::take_view;
    template <typename, std::integral>
    friend class take_backward_iterator;

   private:
    [[no_unique_address]] I i_;
    [[no_unique_address]] Amount amount_;  // the amount left to take

    template <iterator I2>
    constexpr take_forward_iterator(wrapping_construct_t,
                                    I2&& i,
                                    Amount amount) noexcept(std::is_nothrow_constructible_v<I, I2>)
        : i_(std::forward<I2>(i)), amount_(amount) {
        builtin_assume(amount_ >= 0);
    }

   public:
    using index_type = iterator_index_type_t<I>;
    constexpr element_type next() {
        element_type ret = i_.next();
        --amount_;
        return ret;
    }
    // this is for the sentinel produced by inverting a take_forward_iterator
    template <sentinel_for<I> S>
        requires multipass_iterator<I>
    constexpr optional<element_type> next(const take_backward_iterator<S, Amount>& end_i) {
        // it is guaranteed that we will hit end_i before processing `amount` elements, since end_i
        // must have been produced from a take_forward_iterator
        optional<element_type> ret = i_.next(end_i.i_);
        --amount_;
        return ret;
    }
    // this is for the sentinel produced by backward_iter() on the view
    template <sentinel_for<I> S>
    constexpr optional<element_type> next(const take_sentinel<S>& end_i) {
        builtin_assume(amount_ >= 0);
        if (amount_ == 0) {
            return nullopt;
        }
        optional<element_type> ret = i_.next(end_i.i_);
        --amount_;
        return ret;
    }
    constexpr void skip() {
        i_.skip();
        --amount_;
    }
    template <sentinel_for<I> S>
        requires multipass_iterator<I>
    constexpr bool skip(const take_backward_iterator<S, Amount>& end_i) {
        bool res = i_.skip(end_i.i_);
        --amount_;
        return res;
    }
    template <sentinel_for<I> S>
    constexpr bool skip(const take_sentinel<S>& end_i) {
        builtin_assume(amount_ >= 0);
        if (amount_ == 0) {
            return false;
        }
        bool res = i_.skip(end_i.i_);
        --amount_;
        return res;
    }
    constexpr void skip(index_type index)
        requires random_access_iterator<I>
    {
        // UB if (index > amount_) since it runs off the end of the new iterator, which is
        // acceptable
        builtin_assume(index <= amount_);
        i_.skip(index);
        amount_ -= index;
    }
    template <sentinel_for<I> S>
    constexpr index_type skip(index_type index, const take_backward_iterator<S, Amount>& end_i)
        requires random_access_iterator<I>
    {
        index_type actual_amount = i_.skip(index, end_i.i_);
        amount_ -= actual_amount;
        return actual_amount;
    }
    template <sentinel_for<I> S>
    constexpr index_type skip(infinite_t, const take_backward_iterator<S, Amount>& end_i)
        requires random_access_iterator<I>
    {
        // Return type is definitely not infinite_t, since take_backward_iterator can only be
        // obtained from take_forward_iterator.
        index_type actual_amount = i_.skip(infinite_t{}, end_i.i_);
        amount_ -= actual_amount;
        return actual_amount;
    }
    template <sentinel_for<I> S>
    constexpr index_type skip(index_type index, const take_sentinel<S>& end_i)
        requires random_access_iterator<I>
    {
        index_type requested_amount = std::min(amount_, index);
        index_type actual_amount = i_.skip(requested_amount, end_i.i_);
        amount_ -= actual_amount;
        return actual_amount;
    }
    template <sentinel_for<I> S>
    constexpr index_type skip(infinite_t, const take_sentinel<S>& end_i)
        requires random_access_iterator<I>
    {
        index_type actual_amount = i_.skip(amount_, end_i.i_);
        amount_ -= actual_amount;
        return actual_amount;
    }
    constexpr decltype(auto) invert() const
        requires multipass_iterator<I>
    {
        return take_backward_iterator<decltype(i_.invert()), Amount>(
            wrapping_construct, i_.invert(), amount_);
    }
};

template <typename I, std::integral Amount>
class take_backward_iterator {
   private:
    template <forward_view, std::integral>
    friend class duality::take_view;
    template <iterator, std::integral>
    friend class take_forward_iterator;
    [[no_unique_address]] I i_;
    [[no_unique_address]] Amount amount_;  // the amount from the back of the resulting view

    template <typename I2>
    constexpr take_backward_iterator(wrapping_construct_t,
                                     I2&& i,
                                     Amount amount) noexcept(std::is_nothrow_constructible_v<I, I2>)
        : i_(std::forward<I2>(i)), amount_(amount) {
        builtin_assume(amount_ >= 0);
    }
};

template <iterator I, std::integral Amount>
class take_backward_iterator<I, Amount> {
   private:
    using element_type = iterator_element_type_t<I>;
    template <forward_view, std::integral>
    friend class duality::take_view;
    template <iterator, std::integral>
    friend class take_forward_iterator;

   private:
    [[no_unique_address]] I i_;
    [[no_unique_address]] Amount amount_;

    template <iterator I2>
    constexpr take_backward_iterator(wrapping_construct_t,
                                     I2&& i,
                                     Amount amount) noexcept(std::is_nothrow_constructible_v<I, I2>)
        : i_(std::forward<I2>(i)), amount_(amount) {
        builtin_assume(amount_ >= 0);
    }

   public:
    using index_type = iterator_index_type_t<I>;
    constexpr element_type next() {
        element_type ret = i_.next();
        ++amount_;
        return ret;
    }
    template <sentinel_for<I> S>
    constexpr optional<element_type> next(const take_forward_iterator<S, Amount>& end_i) {
        optional<element_type> ret = i_.next(end_i.i_);
        ++amount_;
        return ret;
    }
    constexpr void skip() {
        i_.skip();
        ++amount_;
    }
    template <sentinel_for<I> S>
    constexpr bool skip(const take_forward_iterator<S, Amount>& end_i) {
        bool ret = i_.skip(end_i.i_);
        ++amount_;
        return ret;
    }
    constexpr void skip(index_type index)
        requires random_access_iterator<I>
    {
        i_.skip(index);
        amount_ += index;
    }
    template <sentinel_for<I> S>
    constexpr index_type skip(index_type index, const take_forward_iterator<S, Amount>& end_i)
        requires random_access_iterator<I>
    {
        index_type actual_amount = i_.skip(index, end_i.i_);
        amount_ += actual_amount;
        return actual_amount;
    }
    template <sentinel_for<I> S>
    constexpr index_type skip(infinite_t, const take_forward_iterator<S, Amount>& end_i)
        requires random_access_iterator<I>
    {
        // Return type is definitely not infinite_t, since take_backward_iterator can only be
        // obtained from take_forward_iterator.
        index_type actual_amount = i_.skip(infinite_t{}, end_i.i_);
        amount_ += actual_amount;
        return actual_amount;
    }
    constexpr decltype(auto) invert() const
        requires multipass_iterator<I>
    {
        return take_forward_iterator<decltype(i_.invert()), Amount>(
            wrapping_construct, i_.invert(), amount_);
    }
};

template <typename I>
class take_sentinel {
   private:
    template <forward_view, std::integral>
    friend class duality::take_view;
    template <iterator, std::integral>
    friend class take_forward_iterator;
    [[no_unique_address]] I i_;

    template <typename I2>
    constexpr take_sentinel(wrapping_construct_t,
                            I2&& i) noexcept(std::is_nothrow_constructible_v<I, I2>)
        : i_(std::forward<I2>(i)) {}
};

}  // namespace impl

template <forward_view V, std::integral Amount>
class take_view {
   private:
    [[no_unique_address]] V v_;
    [[no_unique_address]] Amount amount_;

   public:
    template <view V2>
    constexpr take_view(wrapping_construct_t,
                        V2&& v,
                        Amount amount) noexcept(std::is_nothrow_constructible_v<V, V2>)
        : v_(std::forward<V2>(v)), amount_(amount) {
        impl::builtin_assume(amount_ >= 0);
    }
    constexpr decltype(auto) forward_iter() {
        return impl::take_forward_iterator<decltype(v_.forward_iter()), Amount>(
            wrapping_construct, v_.forward_iter(), amount_);
    }
    constexpr decltype(auto) forward_iter() const {
        return impl::take_forward_iterator<decltype(v_.forward_iter()), Amount>(
            wrapping_construct, v_.forward_iter(), amount_);
    }
    constexpr decltype(auto) backward_iter() {
        return impl::take_sentinel<decltype(v_.backward_iter())>(wrapping_construct,
                                                                 v_.backward_iter());
    }
    constexpr decltype(auto) backward_iter() const {
        return impl::take_sentinel<decltype(v_.backward_iter())>(wrapping_construct,
                                                                 v_.backward_iter());
    }
    constexpr bool empty() const
        requires emptyness_view<V>
    {
        impl::builtin_assume(amount_ >= 0);
        return v_.empty() || amount_ == 0;
    }
    constexpr auto size() const
        requires sized_view<V>
    {
        impl::builtin_assume(amount_ >= 0);
        return std::min(v_.size(), static_cast<decltype(v_.size())>(amount_));
    }
    constexpr auto size() const
        requires infinite_view<V>
    {
        impl::builtin_assume(amount_ >= 0);
        return amount_;
    }
};

template <forward_view V2, std::integral Amount>
take_view(wrapping_construct_t, V2&& v, Amount amount) -> take_view<V2, Amount>;

namespace impl {
template <std::integral Amount, std::integral DefaultAmount>
struct take_adaptor {
    template <forward_view V>
    constexpr auto operator()(V&& v) const {
        if constexpr (std::integral<view_index_type_t<V>>) {
            return take_view(
                wrapping_construct, std::forward<V>(v), static_cast<view_index_type_t<V>>(amount));
        } else {
            return take_view(
                wrapping_construct, std::forward<V>(v), static_cast<DefaultAmount>(amount));
        }
    }
    [[no_unique_address]] Amount amount;
};
template <std::integral DefaultAmount>
struct take {
    template <forward_view V>
        requires std::same_as<view_index_type_t<V>, no_index_type_t>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v, DefaultAmount amount) DUALITY_CONST_CALL {
        return take_view(wrapping_construct, std::forward<V>(v), amount);
    }
    template <forward_view V>
        requires std::integral<view_index_type_t<V>>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v,
                                                  view_index_type_t<V> amount) DUALITY_CONST_CALL {
        return take_view(wrapping_construct, std::forward<V>(v), amount);
    }
    template <std::integral Amount>
    constexpr DUALITY_STATIC_CALL auto operator()(Amount amount) DUALITY_CONST_CALL {
        return take_adaptor<Amount, DefaultAmount>{amount};
    }
};
}  // namespace impl

namespace views {
constexpr inline impl::take<std::size_t> take;
}

}  // namespace duality
