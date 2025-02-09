// This file is part of https://github.com/btzy/duality
#pragma once

#include <type_traits>
#include <utility>

#include <duality/core_view.hpp>
#include <duality/range.hpp>

/// Implementation for actions::reverse.  Modifies the given view by reversing the elements
/// in-place.  If the view is multipass, returns a range containing the original view (which now
/// contains modified elements).
///
/// E.g.:
/// reverse({1, 3, 5, 7, 9}) -> {9, 7, 5, 3, 1}
///
/// actions::reverse only accepts bidirectional views.  If the given view is multipass, the returned
/// range has the same iterators as the original view, i.e. actions::reverse is iterator-preserving.

namespace duality {

namespace impl {

template <typename ForwardIter, typename BackwardIter>
inline void reverse_action_iters(ForwardIter& fit, BackwardIter& bit) {
    while (true) {
        optional first_opt = fit.next(bit);
        if (!first_opt) break;
        optional last_opt = bit.next(fit);
        if (!last_opt) break;
        using std::swap;
        swap(*first_opt, *last_opt);
    }
}

template <bidirectional_view V>
inline auto reverse_action(V&& v) {
    auto fit = v.forward_iter();
    auto bit = v.backward_iter();
    if constexpr (multipass_bidirectional_view<V>) {
        auto fit_clone = fit;
        auto bit_clone = bit;
        reverse_action_iters(fit_clone, bit_clone);
        return range(std::move(fit), std::move(bit));
    } else {
        reverse_action_iter(fit, bit);
    }
}

struct reverse_a_adaptor {
    template <bidirectional_view V>
        requires std::is_lvalue_reference_v<view_element_type_t<V>>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v) DUALITY_CONST_CALL {
        return reverse_action(std::forward<V>(v));
    }
};
struct reverse_a {
    template <bidirectional_view V>
        requires std::is_lvalue_reference_v<view_element_type_t<V>>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v) DUALITY_CONST_CALL {
        return reverse_action(std::forward<V>(v));
    }
    constexpr DUALITY_STATIC_CALL auto operator()() DUALITY_CONST_CALL {
        return reverse_a_adaptor{};
    }
};

}  // namespace impl

namespace actions {
constexpr inline impl::reverse_a reverse;
}

}  // namespace duality
