// This file is part of https://github.com/btzy/duality
#pragma once

#include <type_traits>
#include <utility>

#include <duality/core_view.hpp>

namespace duality {

template <view V>
class reverse_view {
   private:
    [[no_unique_address]] V v_;

   public:
    template <view V2>
    constexpr reverse_view(wrapping_construct_t,
                           V2&& v) noexcept(std::is_nothrow_constructible_v<V, V2>)
        : v_(std::forward<V2>(v)) {}
    constexpr decltype(auto) forward_iter() { return v_.backward_iter(); }
    constexpr decltype(auto) forward_iter() const { return v_.backward_iter(); }
    constexpr decltype(auto) backward_iter() { return v_.forward_iter(); }
    constexpr decltype(auto) backward_iter() const { return v_.forward_iter(); }
    constexpr decltype(auto) empty() const
        requires emptyness_view<V>
    {
        return v_.empty();
    }
    constexpr decltype(auto) size() const
        requires sized_view<V> || infinite_view<V>
    {
        return v_.size();
    }
};

template <bidirectional_view V2>
reverse_view(wrapping_construct_t, V2&& v) -> reverse_view<V2>;

namespace impl {
struct reverse_adaptor {
    template <bidirectional_view V>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v) DUALITY_CONST_CALL {
        return reverse_view(wrapping_construct, std::forward<V>(v));
    }
};
struct reverse {
    template <bidirectional_view V>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v) DUALITY_CONST_CALL {
        return reverse_view(wrapping_construct, std::forward<V>(v));
    }
    constexpr DUALITY_STATIC_CALL auto operator()() DUALITY_CONST_CALL { return reverse_adaptor{}; }
};
}  // namespace impl

namespace views {
constexpr inline impl::reverse reverse;
}

}  // namespace duality
