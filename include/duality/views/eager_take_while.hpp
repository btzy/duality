// This file is part of https://github.com/btzy/duality
#pragma once

#include <concepts>
#include <utility>

#include <duality/core_view.hpp>

/// Implementation for views::eager_take_while.  Truncates the range upon encountering the first
/// element that does not satisfy the given predicate.
///
/// E.g.:
/// eager_take_while({1, 3, 5, 7, 9}, [](int x) { return x < 6; }) -> {1, 3, 5}
///
/// views::eager_take_while accepts only multipass_forward_views.  This view is iterator-preserving,
/// meaning that eager_take_while_view does not define any new iterator type; iterators of an
/// eager_take_while view are the same type as some iterators of the underlying view.  The forward
/// iterator of the eager_take_while view is the same type as the forward iterator of the underlying
/// view, and the backward iterator is the same type as the inverted forward iterator of the
/// underlying view.
///
/// This view differs from lazy_take_while in what backward_iter() does.  eager_take_while computes
/// the position of the backward iterator in the call to backward_iter().  This is a linear time
/// operation.

namespace duality {

namespace impl {
template <typename F, typename T>
concept eager_take_while_function = std::predicate<F, T&>;
}

template <multipass_forward_view V, impl::eager_take_while_function<view_element_type_t<V>> F>
class eager_take_while_view {
   private:
    V v_;
    F f_;

   public:
    template <multipass_forward_view V2, impl::eager_take_while_function<view_element_type_t<V>> F2>
    constexpr eager_take_while_view(wrapping_construct_t, V2&& v, F2&& f) noexcept(
        std::is_nothrow_constructible_v<V, V2> && std::is_nothrow_constructible_v<F, F2>)
        : v_(std::forward<V2>(v)), f_(std::forward<F2>(f)) {}
    constexpr decltype(auto) forward_iter() { return v_.forward_iter(); }
    constexpr decltype(auto) forward_iter() const { return v_.forward_iter(); }
    constexpr decltype(auto) backward_iter() {
        auto fit = v_.forward_iter();
        auto bit = v_.backward_iter();
        while (true) {
            auto prev_fit = fit;
            auto opt = fit.next(bit);
            if (!opt || !f_(*opt)) {
                return prev_fit.invert();
            }
        }
    }
    constexpr decltype(auto) backward_iter() const {
        auto fit = v_.forward_iter();
        auto bit = v_.backward_iter();
        while (true) {
            auto prev_fit = fit;
            auto opt = fit.next(bit);
            if (!opt || !f_(*opt)) {
                return prev_fit.invert();
            }
        }
    }
};

template <multipass_forward_view V2, impl::eager_take_while_function<view_element_type_t<V2>> F2>
eager_take_while_view(wrapping_construct_t, V2&& v, F2&& f) -> eager_take_while_view<V2, F2>;

namespace impl {
template <typename F>
struct eager_take_while_adaptor {
    template <view V>
        requires impl::eager_take_while_function<F, view_element_type_t<V>>
    constexpr auto operator()(V&& v) const& {
        return eager_take_while_view(wrapping_construct, std::forward<V>(v), f);
    }
    template <view V>
        requires impl::eager_take_while_function<F, view_element_type_t<V>>
    constexpr auto operator()(V&& v) && {
        return eager_take_while_view(wrapping_construct, std::forward<V>(v), std::forward<F>(f));
    }
    [[no_unique_address]] F f;
};
struct eager_take_while {
    template <view V, impl::eager_take_while_function<view_element_type_t<V>> F>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v, F&& f) DUALITY_CONST_CALL {
        return eager_take_while_view(wrapping_construct, std::forward<V>(v), std::forward<F>(f));
    }
    template <typename F>
    constexpr DUALITY_STATIC_CALL auto operator()(F&& f) DUALITY_CONST_CALL {
        return eager_take_while_adaptor<F>{std::forward<F>(f)};
    }
};
}  // namespace impl

namespace views {
constexpr inline impl::eager_take_while eager_take_while;
}

}  // namespace duality
