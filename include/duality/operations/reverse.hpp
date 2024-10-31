// This file is part of https://github.com/btzy/duality
#pragma once

// Operation that reverses the data referenced by the view and returns the view itself.

#include <utility>

#include <duality/builtin_assume.hpp>
#include <duality/core_view.hpp>
#include <duality/view_ops.hpp>

namespace duality {

namespace impl {
/// @brief Reverses the given view.
/// @tparam V The type of view.
/// @param v The view.
/// @return The given view.
template <dual_view V>
    requires std::is_lvalue_reference_v<views::element_type_t<V>>
constexpr auto reverse_operation(V&& v) {
    if constexpr (sized_view<V>) {
        auto size = view_ops::size(static_cast<const V&>(v));
        auto&& dual_stream = view_ops::dual_stream(v);
        while (size >= 2) {
            auto next = dual_stream.next();
            builtin_assume(static_cast<bool>(next));
            auto prev = dual_stream.prev();
            builtin_assume(static_cast<bool>(prev));
            using std::swap;
            swap(*next, *prev);
            size -= 2;
        }
    } else {
        auto&& dual_stream = view_ops::dual_stream(v);
        while (true) {
            auto next = dual_stream.next();
            if (!next) break;
            auto prev = dual_stream.prev();
            if (!prev) break;
            using std::swap;
            swap(*next, *prev);
        }
    }
    return std::forward<V>(v);
}

struct reverse_op_adaptor {
    template <dual_view V>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v) DUALITY_CONST_CALL {
        return reverse_operation(std::forward<V>(v));
    }
};
struct reverse_op {
    template <dual_view V>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v) DUALITY_CONST_CALL {
        return reverse_operation(std::forward<V>(v));
    }
    constexpr DUALITY_STATIC_CALL auto operator()() DUALITY_CONST_CALL {
        return reverse_op_adaptor{};
    }
};
}  // namespace impl

namespace operations {
constexpr inline impl::reverse_op reverse;
}

}  // namespace duality
