// This file is part of https://github.com/btzy/duality
#pragma once

#include <algorithm>
#include <concepts>
#include <utility>

#include <duality/builtin_assume.hpp>
#include <duality/core_view.hpp>

/// Implementation for views::eager_take.  Truncates the range after the given number of elements.
///
/// E.g.:
/// eager_take({1, 3, 5, 7, 9}, 3) -> {1, 3, 5}
///
/// views::eager_take accepts only multipass_forward_views.  This view is iterator-preserving,
/// meaning that eager_take_view does not define any new iterator type; iterators of an eager_take
/// view are the same type as some iterators of the underlying view.  The forward iterator of the
/// eager_take view is the same type as the forward iterator of the underlying view, and the
/// backward iterator is the same type as the inverted forward iterator of the underlying view.
///
/// This view differs from lazy_take in what backward_iter() does.  eager_take computes the position
/// of the backward iterator in the call to backward_iter().  This is constant time for iterators
/// that satisfy random_access_iterator, but is linear time otherwise.

namespace duality {

template <multipass_forward_view V, std::integral Amount>
class eager_take_view {
   private:
    [[no_unique_address]] V v_;
    [[no_unique_address]] Amount amount_;

   public:
    template <multipass_forward_view V2>
    constexpr eager_take_view(wrapping_construct_t,
                              V2&& v,
                              Amount amount) noexcept(std::is_nothrow_constructible_v<V, V2>)
        : v_(std::forward<V2>(v)), amount_(amount) {
        impl::builtin_assume(amount_ >= 0);
    }
    constexpr decltype(auto) forward_iter() { return v_.forward_iter(); }
    constexpr decltype(auto) forward_iter() const { return v_.forward_iter(); }
    constexpr decltype(auto) backward_iter() {
        auto fit = v_.forward_iter();
        auto bit = v_.backward_iter();
        for (Amount amt = amount_; amt > 0; --amt) {
            auto prev_fit = fit;
            if (!fit.skip(bit)) {
                // After a failed skip(bit) operation, fit is in an unspecified (but valid) state,
                // so we cannot use it.  Instead, we invert a previously cached iterator from before
                // the skip operation.
                return prev_fit.invert();
            }
        }
        return fit.invert();
    }
    constexpr decltype(auto) backward_iter()
        requires random_access_iterator<decltype(v_.forward_iter())>
    {
        auto fit = v_.forward_iter();
        fit.skip(amount_, v_.backward_iter());
        return fit.invert();
    }
    constexpr decltype(auto) backward_iter() const {
        auto fit = v_.forward_iter();
        auto bit = v_.backward_iter();
        for (Amount amt = amount_; amt > 0; --amt) {
            auto prev_fit = fit;
            if (!fit.skip(bit)) {
                // After a failed skip(bit) operation, fit is in an unspecified (but valid) state,
                // so we cannot use it.  Instead, we invert a previously cached iterator from before
                // the skip operation.
                return prev_fit.invert();
            }
        }
        return fit.invert();
    }
    constexpr decltype(auto) backward_iter() const
        requires random_access_iterator<decltype(v_.forward_iter())>
    {
        auto fit = v_.forward_iter();
        fit.skip(amount_, v_.backward_iter());
        return fit.invert();
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

template <multipass_forward_view V2, std::integral Amount>
eager_take_view(wrapping_construct_t, V2&& v, Amount amount) -> eager_take_view<V2, Amount>;

namespace impl {
template <std::integral Amount = std::size_t>
struct eager_take_adaptor {
    template <multipass_forward_view V>
    constexpr auto operator()(V&& v) const {
        if constexpr (std::integral<view_index_type_t<V>>) {
            return eager_take_view(
                wrapping_construct, std::forward<V>(v), static_cast<view_index_type_t<V>>(amount));
        } else {
            return eager_take_view(wrapping_construct, std::forward<V>(v), amount);
        }
    }
    [[no_unique_address]] Amount amount;
};
struct eager_take {
    template <multipass_forward_view V>
        requires std::same_as<view_index_type_t<V>, no_index_type_t>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v, size_t amount) DUALITY_CONST_CALL {
        return eager_take_view(wrapping_construct, std::forward<V>(v), amount);
    }
    template <multipass_forward_view V>
        requires std::integral<view_index_type_t<V>>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v,
                                                  view_index_type_t<V> amount) DUALITY_CONST_CALL {
        return eager_take_view(wrapping_construct, std::forward<V>(v), amount);
    }
    constexpr DUALITY_STATIC_CALL auto operator()(size_t amount) DUALITY_CONST_CALL {
        return eager_take_adaptor<size_t>{amount};
    }
};
}  // namespace impl

namespace views {
constexpr inline impl::eager_take eager_take;
}

}  // namespace duality
