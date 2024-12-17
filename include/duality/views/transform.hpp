// This file is part of https://github.com/btzy/duality
#pragma once

#include <concepts>
#include <utility>

#include <duality/core_view.hpp>

namespace duality {

namespace impl {
template <typename F, typename T>
concept transform_function = std::invocable<F, T> && std::move_constructible<F>;
}

template <view V, impl::transform_function<view_element_type_t<V>> F>
class transform_view;

namespace impl {

template <typename I, std::move_constructible F>
class transform_iterator {
   private:
    template <view V, transform_function<view_element_type_t<V>>>
    friend class duality::transform_view;
    template <typename, std::move_constructible>
    friend class transform_iterator;
    [[no_unique_address]] I i_;

    template <typename I2>
    constexpr transform_iterator(wrapping_construct_t,
                                 I2&& i,
                                 F&) noexcept(std::is_nothrow_constructible_v<I, I2>)
        : i_(std::forward<I2>(i)) {}
};

template <iterator I, transform_function<iterator_element_type_t<I>> F>
class transform_iterator<I, F> {
   private:
    using element_type = std::invoke_result_t<F, iterator_element_type_t<I>>;
    template <view V, transform_function<view_element_type_t<V>>>
    friend class duality::transform_view;
    template <typename, std::move_constructible>
    friend class transform_iterator;
    [[no_unique_address]] I i_;
    F* f_;

    template <iterator I2>
    constexpr transform_iterator(wrapping_construct_t,
                                 I2&& i,
                                 F& f) noexcept(std::is_nothrow_constructible_v<I, I2>)
        : i_(std::forward<I2>(i)), f_(&f) {}

   public:
    using index_type = iterator_index_type_t<I>;
    constexpr element_type next() { return (*f_)(i_.next()); }
    template <sentinel_for<I> S>
    constexpr optional<element_type> next(const transform_iterator<S, F>& end_i) {
        if (auto opt = i_.next(end_i.i_)) {
            return (*f_)(*std::move(opt));
        }
        return nullopt;
    }
    constexpr void skip() { i_.skip(); }
    template <sentinel_for<I> S>
    constexpr bool skip(const transform_iterator<S, F>& end_i) {
        return i_.skip(end_i.i_);
    }
    constexpr void skip(iterator_index_type_t<I> index)
        requires random_access_iterator<I>
    {
        i_.skip(index);
    }
    template <sentinel_for<I> S>
    constexpr iterator_index_type_t<I> skip(iterator_index_type_t<I> index,
                                            const transform_iterator<S, F>& end_i)
        requires random_access_iterator<I>
    {
        return i_.skip(index, end_i.i_);
    }
    constexpr decltype(auto) invert() const
        requires multipass_iterator<I>
    {
        return transform_iterator<decltype(i_.invert()), F>(wrapping_construct, i_.invert(), *f_);
    }
};

}  // namespace impl

template <view V, impl::transform_function<view_element_type_t<V>> F>
class transform_view {
   private:
    [[no_unique_address]] V v_;
    [[no_unique_address]] F f_;

   public:
    template <view V2, impl::transform_function<view_element_type_t<V>> F2>
    constexpr transform_view(wrapping_construct_t,
                             V2&& v,
                             F2&& f) noexcept(std::is_nothrow_constructible_v<V, V2>)
        : v_(std::forward<V2>(v)), f_(std::forward<F2>(f)) {}
    constexpr decltype(auto) forward_iter() {
        return impl::transform_iterator<decltype(v_.forward_iter()),
                                        std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.forward_iter(), f_);
    }
    constexpr decltype(auto) forward_iter() const {
        return impl::transform_iterator<decltype(v_.forward_iter()),
                                        std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.forward_iter(), f_);
    }
    constexpr decltype(auto) backward_iter() {
        return impl::transform_iterator<decltype(v_.backward_iter()),
                                        std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.backward_iter(), f_);
    }
    constexpr decltype(auto) backward_iter() const {
        return impl::transform_iterator<decltype(v_.backward_iter()),
                                        std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.backward_iter(), f_);
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

template <view V2, impl::transform_function<view_element_type_t<V2>> F2>
transform_view(wrapping_construct_t, V2&& v, F2&& f) -> transform_view<V2, F2>;

namespace impl {
template <typename F>
struct transform_adaptor {
    template <view V>
        requires impl::transform_function<F, view_element_type_t<V>>
    constexpr auto operator()(V&& v) const& {
        return transform_view(wrapping_construct, std::forward<V>(v), f);
    }
    template <view V>
        requires impl::transform_function<F, view_element_type_t<V>>
    constexpr auto operator()(V&& v) && {
        return transform_view(wrapping_construct, std::forward<V>(v), std::forward<F>(f));
    }
    [[no_unique_address]] F f;
};
struct transform {
    template <view V, impl::transform_function<view_element_type_t<V>> F>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v, F&& f) DUALITY_CONST_CALL {
        return transform_view(wrapping_construct, std::forward<V>(v), std::forward<F>(f));
    }
    template <typename F>
    constexpr DUALITY_STATIC_CALL auto operator()(F&& f) DUALITY_CONST_CALL {
        return transform_adaptor<F>{std::forward<F>(f)};
    }
};
}  // namespace impl

namespace views {
constexpr inline impl::transform transform;
}

}  // namespace duality
