// This file is part of https://github.com/btzy/duality
#pragma once

// This is not really a view as it doesn't satisfy forward_view, but it allows conversion to
// something that satisfies std::input_range, so that it can be used with Ranges.

#include <iterator>
#include <utility>

#include <duality/core_view.hpp>

namespace duality {

template <forward_view V>
class as_input_range_view;

namespace impl {

class as_input_range_sentinel {};

template <iterator I, sentinel_for<I> S>
class as_input_range_iterator {
   private:
    template <forward_view V>
    friend class duality::as_input_range_view;
    using element_type = iterator_element_type_t<I>;
    [[no_unique_address]] I i_;
    [[no_unique_address]] S s_;
    [[no_unique_address]] alignas(
        optional<element_type>) mutable char cache_[sizeof(optional<element_type>)];

    constexpr as_input_range_iterator(I&& i, S&& s) : i_(std::move(i)), s_(std::move(s)) {
        construct_next();
    }

    constexpr void construct_next() {
        std::construct_at(reinterpret_cast<optional<element_type>*>(cache_), i_.next(s_));
    }

    constexpr void copy_next(optional<element_type>&& other) noexcept(
        std::is_nothrow_move_constructible_v<element_type>) {
        std::construct_at(reinterpret_cast<optional<element_type>*>(cache_), std::move(other));
    }

    constexpr void destroy_next() noexcept {
        std::destroy_at(std::launder(reinterpret_cast<optional<element_type>*>(cache_)));
    }

   public:
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_cvref_t<element_type>;
    using pointer = std::add_pointer_t<std::remove_reference_t<element_type>>;
    using reference = std::add_lvalue_reference_t<std::remove_reference_t<element_type>>;

    constexpr ~as_input_range_iterator() { destroy_next(); }
    constexpr as_input_range_iterator(as_input_range_iterator&& other) noexcept(
        std::is_nothrow_move_constructible_v<I> &&
        std::is_nothrow_move_constructible_v<element_type>)
        : i_(std::move(other.i_)) {
        copy_next(
            std::move(*std::launder(reinterpret_cast<optional<element_type>*>(other.cache_))));
    }
    constexpr as_input_range_iterator& operator=(as_input_range_iterator&& other) noexcept(
        std::is_nothrow_move_assignable_v<I> &&
        std::is_nothrow_move_constructible_v<element_type>) {
        destroy_next();
        i_ = std::move(other.i_);
        copy_next(
            std::move(*std::launder(reinterpret_cast<optional<element_type>*>(other.cache_))));
    }
    constexpr decltype(auto) operator*() const noexcept {
        return *std::move(*std::launder(reinterpret_cast<optional<element_type>*>(cache_)));
    }
    constexpr as_input_range_iterator& operator++() {
        destroy_next();
        construct_next();
        return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr bool operator==(as_input_range_sentinel) const noexcept {
        return !*std::launder(reinterpret_cast<const optional<element_type>*>(cache_));
    }
};
static_assert(
    std::input_iterator<as_input_range_iterator<dummy_forward_random_access_iterator<int>,
                                                dummy_backward_random_access_iterator<int>>>);
static_assert(
    std::sentinel_for<as_input_range_sentinel,
                      as_input_range_iterator<dummy_forward_random_access_iterator<int>,
                                              dummy_backward_random_access_iterator<int>>>);

}  // namespace impl

template <forward_view V>
class as_input_range_view {
   private:
    V v_;

   public:
    template <forward_view V2>
    constexpr as_input_range_view(wrapping_construct_t,
                                  V2&& v) noexcept(std::is_nothrow_constructible_v<V, V2>)
        : v_(std::forward<V2>(v)) {}
    constexpr auto begin() {
        return impl::as_input_range_iterator<decltype(v_.forward_iter()),
                                             decltype(v_.backward_iter())>(v_.forward_iter(),
                                                                           v_.backward_iter());
    }
    constexpr static auto end() noexcept { return impl::as_input_range_sentinel{}; }
};

template <forward_view V2>
as_input_range_view(wrapping_construct_t, V2&& v) -> as_input_range_view<V2>;

namespace impl {
struct as_input_range_adaptor {
    template <forward_view V>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v) DUALITY_CONST_CALL {
        return as_input_range_view(wrapping_construct, std::forward<V>(v));
    }
};
struct as_input_range {
    template <forward_view V>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v) DUALITY_CONST_CALL {
        return as_input_range_view(wrapping_construct, std::forward<V>(v));
    }
    constexpr DUALITY_STATIC_CALL auto operator()() DUALITY_CONST_CALL {
        return as_input_range_adaptor{};
    }
};
}  // namespace impl

namespace views {
constexpr inline impl::as_input_range as_input_range;
}

}  // namespace duality
