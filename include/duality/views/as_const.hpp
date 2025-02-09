// This file is part of https://github.com/btzy/duality
#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#include <duality/core_view.hpp>

/// Implementation for views::as_const.  Converts elements yielded to const references.  Requires
/// that the original view yields some kind of reference (either lvalue or rvalue, and either const
/// or non-const).  If the view is already const, then this adaptor yields the original view.
///
/// All view concepts satisifed by the original view are also satisfied by the resultant view.

namespace duality {

template <view V>
class as_const_view;

namespace impl {

template <typename T>
struct add_const_to_reference;

template <typename T>
    requires std::is_lvalue_reference_v<T>
struct add_const_to_reference<T> {
    using type = std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>;
};
template <typename T>
    requires std::is_rvalue_reference_v<T>
struct add_const_to_reference<T> {
    using type = std::add_rvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>;
};
template <typename T>
using add_const_to_reference_t = typename add_const_to_reference<T>::type;

template <typename I>
class as_const_iterator {
   private:
    template <view V>
    friend class duality::as_const_view;
    template <typename>
    friend class as_const_iterator;
    [[no_unique_address]] I i_;

    template <typename I2>
    constexpr as_const_iterator(wrapping_construct_t,
                                I2&& i) noexcept(std::is_nothrow_constructible_v<I, I2>)
        : i_(std::forward<I2>(i)) {}
};

template <iterator I>
class as_const_iterator<I> {
   private:
    using element_type = add_const_to_reference_t<iterator_element_type_t<I>>;
    template <view V>
    friend class duality::as_const_view;
    template <typename>
    friend class as_const_iterator;
    [[no_unique_address]] I i_;

    template <iterator I2>
    constexpr as_const_iterator(wrapping_construct_t,
                                I2&& i) noexcept(std::is_nothrow_constructible_v<I, I2>)
        : i_(std::forward<I2>(i)) {}

   public:
    using index_type = iterator_index_type_t<I>;
    constexpr element_type next() { return i_.next(); }
    template <sentinel_for<I> S>
    constexpr optional<element_type> next(const as_const_iterator<S>& end_i) {
        return i_.next(end_i.i_);
    }
    constexpr void skip() { i_.skip(); }
    template <sentinel_for<I> S>
    constexpr bool skip(const as_const_iterator<S>& end_i) {
        return i_.skip(end_i.i_);
    }
    constexpr void skip(iterator_index_type_t<I> index)
        requires random_access_iterator<I>
    {
        i_.skip(index);
    }
    template <sentinel_for<I> S>
    constexpr iterator_index_type_t<I> skip(iterator_index_type_t<I> index,
                                            const as_const_iterator<S>& end_i)
        requires random_access_iterator<I>
    {
        return i_.skip(index, end_i.i_);
    }
    template <sentinel_for<I> S>
    constexpr auto skip(infinite_t, const as_const_iterator<S>& end_i)
        requires random_access_iterator<I>
    {
        return i_.skip(infinite_t{}, end_i.i_);
    }
    constexpr decltype(auto) invert() const
        requires multipass_iterator<I>
    {
        return as_const_iterator<decltype(i_.invert())>(wrapping_construct, i_.invert());
    }
};

}  // namespace impl

template <view V>
class as_const_view {
   private:
    [[no_unique_address]] V v_;

   public:
    template <view V2>
    constexpr as_const_view(wrapping_construct_t,
                            V2&& v) noexcept(std::is_nothrow_constructible_v<V, V2>)
        : v_(std::forward<V2>(v)) {}
    constexpr decltype(auto) forward_iter() {
        using element_type = impl::add_const_to_reference_t<view_element_type_t<V>>;
        if constexpr (std::same_as<element_type, view_element_type_t<V>>) {
            return v_.forward_iter();
        } else {
            return impl::as_const_iterator<decltype(v_.forward_iter())>(wrapping_construct,
                                                                        v_.forward_iter());
        }
    }
    constexpr decltype(auto) forward_iter() const {
        using element_type = impl::add_const_to_reference_t<view_element_type_t<const V>>;
        if constexpr (std::same_as<element_type, view_element_type_t<const V>>) {
            return v_.forward_iter();
        } else {
            return impl::as_const_iterator<decltype(v_.forward_iter())>(wrapping_construct,
                                                                        v_.forward_iter());
        }
    }
    constexpr decltype(auto) backward_iter() {
        using element_type = impl::add_const_to_reference_t<view_element_type_t<V>>;
        if constexpr (std::same_as<element_type, view_element_type_t<V>>) {
            return v_.backward_iter();
        } else {
            return impl::as_const_iterator<decltype(v_.backward_iter())>(wrapping_construct,
                                                                         v_.backward_iter());
        }
    }
    constexpr decltype(auto) backward_iter() const {
        using element_type = impl::add_const_to_reference_t<view_element_type_t<const V>>;
        if constexpr (std::same_as<element_type, view_element_type_t<const V>>) {
            return v_.backward_iter();
        } else {
            return impl::as_const_iterator<decltype(v_.backward_iter())>(wrapping_construct,
                                                                         v_.backward_iter());
        }
    }
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

template <view V2>
as_const_view(wrapping_construct_t, V2&& v) -> as_const_view<V2>;

namespace impl {
struct as_const_adaptor {
    template <view V>
        requires std::is_reference_v<view_element_type_t<V>>
    constexpr DUALITY_STATIC_CALL decltype(auto) operator()(V&& v) DUALITY_CONST_CALL {
        if constexpr (std::is_const_v<std::remove_reference_t<view_element_type_t<V>>>) {
            return std::forward<V>(v);
        } else {
            return as_const_view(wrapping_construct, std::forward<V>(v));
        }
    }
};
struct as_const {
    template <view V>
        requires std::is_reference_v<view_element_type_t<V>>
    constexpr DUALITY_STATIC_CALL decltype(auto) operator()(V&& v) DUALITY_CONST_CALL {
        if constexpr (std::is_const_v<std::remove_reference_t<view_element_type_t<V>>>) {
            return std::forward<V>(v);
        } else {
            return as_const_view(wrapping_construct, std::forward<V>(v));
        }
    }
    constexpr DUALITY_STATIC_CALL auto operator()() DUALITY_CONST_CALL {
        return as_const_adaptor{};
    }
};
}  // namespace impl

namespace views {
constexpr inline impl::as_const as_const;
}

}  // namespace duality
