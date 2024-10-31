// This file is part of https://github.com/btzy/duality
#pragma once

#include <concepts>
#include <utility>

#include <duality/core_view.hpp>

namespace duality {

namespace impl {
template <typename F, typename T>
concept filter_function = std::predicate<F, T&> && std::move_constructible<F>;
}

template <view V, impl::filter_function<view_element_type_t<V>> F>
class filter_view;

namespace impl {

template <typename I, std::move_constructible F>
class filter_iterator {
   private:
    template <view V, filter_function<view_element_type_t<V>>>
    friend class duality::filter_view;
    template <typename, std::move_constructible>
    friend class filter_iterator;
    [[no_unique_address]] I i_;

    template <typename I2>
    constexpr filter_iterator(wrapping_construct_t,
                              I2&& i,
                              F&) noexcept(std::is_nothrow_constructible_v<I, I2>)
        : i_(std::forward<I2>(i)) {}
};

template <iterator I, filter_function<iterator_element_type_t<I>> F>
class filter_iterator<I, F> {
   private:
    using element_type = iterator_element_type_t<I>;
    template <view V, filter_function<view_element_type_t<V>>>
    friend class duality::filter_view;
    template <typename, std::move_constructible>
    friend class filter_iterator;
    [[no_unique_address]] I i_;
    F* f_;

    template <iterator I2>
    constexpr filter_iterator(wrapping_construct_t,
                              I2&& i,
                              F& f) noexcept(std::is_nothrow_constructible_v<I, I2>)
        : i_(std::forward<I2>(i)), f_(&f) {}

   public:
    using index_type = no_index_type_t;
    constexpr element_type next() {
        while (true) {
            element_type val = i_.next();
            if ((*f_)(val)) return val;
        }
    }
    template <sentinel_for<I> S>
    constexpr optional<element_type> next(const filter_iterator<S, F>& end_i) {
        while (optional<element_type> opt = i_.next(end_i.i_)) {
            if ((*f_)(*opt)) return opt;
        }
        return nullopt;
    }
    constexpr void skip() {
        while (true) {
            element_type val = i_.next();
            if ((*f_)(val)) return;
        }
    }
    template <sentinel_for<I> S>
    constexpr bool skip(const filter_iterator<S, F>& end_i) {
        while (optional<element_type> opt = i_.next(end_i.i_)) {
            if ((*f_)(*opt)) return true;
        }
        return false;
    }
    constexpr decltype(auto) invert() const
        requires multipass_iterator<I>
    {
        return filter_iterator<decltype(i_.invert()), F>(wrapping_construct, i_.invert(), *f_);
    }
};

}  // namespace impl

template <view V, impl::filter_function<view_element_type_t<V>> F>
class filter_view {
   private:
    V v_;
    F f_;

   public:
    using index_type = no_index_type_t;
    template <view V2, impl::filter_function<view_element_type_t<V>> F2>
    constexpr filter_view(wrapping_construct_t,
                          V2&& v,
                          F2&& f) noexcept(std::is_nothrow_constructible_v<V, V2>)
        : v_(std::forward<V2>(v)), f_(std::forward<F2>(f)) {}
    constexpr decltype(auto) forward_iter() {
        return impl::filter_iterator<decltype(v_.forward_iter()),
                                     std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.forward_iter(), f_);
    }
    constexpr decltype(auto) forward_iter() const {
        return impl::filter_iterator<decltype(v_.forward_iter()),
                                     std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.forward_iter(), f_);
    }
    constexpr decltype(auto) backward_iter() {
        return impl::filter_iterator<decltype(v_.backward_iter()),
                                     std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.backward_iter(), f_);
    }
    constexpr decltype(auto) backward_iter() const {
        return impl::filter_iterator<decltype(v_.backward_iter()),
                                     std::remove_reference_t<decltype((f_))>>(
            wrapping_construct, v_.backward_iter(), f_);
    }
};

template <view V2, impl::filter_function<view_element_type_t<V2>> F2>
filter_view(wrapping_construct_t, V2&& v, F2&& f) -> filter_view<V2, F2>;

namespace impl {
template <typename F>
struct filter_adaptor {
    template <view V>
        requires impl::filter_function<F, view_element_type_t<V>>
    constexpr auto operator()(V&& v) const& {
        return filter_view(wrapping_construct, std::forward<V>(v), f);
    }
    template <view V>
        requires impl::filter_function<F, view_element_type_t<V>>
    constexpr auto operator()(V&& v) && {
        return filter_view(wrapping_construct, std::forward<V>(v), std::forward<F>(f));
    }
    [[no_unique_address]] F f;
};
struct filter {
    template <view V, impl::filter_function<view_element_type_t<V>> F>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v, F&& f) DUALITY_CONST_CALL {
        return filter_view(wrapping_construct, std::forward<V>(v), std::forward<F>(f));
    }
    template <typename F>
    constexpr DUALITY_STATIC_CALL auto operator()(F&& f) DUALITY_CONST_CALL {
        return filter_adaptor<F>{std::forward<F>(f)};
    }
};
}  // namespace impl

namespace views {
constexpr inline impl::filter filter;
}

}  // namespace duality
