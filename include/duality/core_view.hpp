// This file is part of https://github.com/btzy/duality
#pragma once

// This file contains all the core view concepts.  In general, when a view is passed another view by
// lvalue, it will old a reference to the other view.  If passed by rvalue, it will move the other
// view into itself.

#include <concepts>
#include <new>
#include <type_traits>

// Note: compatibility and wrapping_construct aren't actually used in this file, but they're
// included here so that individual view implementations don't need to include too many different
// headers.
#include <duality/compatibility.hpp>
#include <duality/core_iterator.hpp>
#include <duality/wrapping_construct.hpp>

namespace duality {

template <typename V>
concept const_iterable_view = std::move_constructible<std::remove_cvref_t<V>> &&
                              (
                                  requires(const V& v) {
                                      { v.forward_iter() } -> iterator;
                                      {
                                          v.backward_iter()
                                      } -> sentinel_for<decltype(v.forward_iter())>;
                                  } ||
                                  requires(const V& v) {
                                      {
                                          v.forward_iter()
                                      } -> sentinel_for<decltype(v.backward_iter())>;
                                      { v.backward_iter() } -> iterator;
                                  });

/// A forward_view is a view whose elements can be streamed in sequence.
template <typename V>
concept forward_view = std::move_constructible<std::remove_cvref_t<V>> && requires(V&& v) {
    { v.forward_iter() } -> iterator;
    { v.backward_iter() } -> sentinel_for<decltype(v.forward_iter())>;
};

/// A backward_view is a view whose elements can be streamed in sequence backwards.  There are
/// unlikely to be any containers that can be converted to a backward_view that is not also a
/// forward_view, but this may be synthesised by reversing a forward_view.
template <typename V>
concept backward_view = std::move_constructible<std::remove_cvref_t<V>> && requires(V&& v) {
    { v.backward_iter() } -> iterator;
    { v.forward_iter() } -> sentinel_for<decltype(v.backward_iter())>;
};

/// This is the weakest view requirement.  No view or operation requires exactly this concept, but
/// this concept is used to constrain some things that should only work with views.
template <typename V>
concept view = forward_view<V> || backward_view<V>;

template <typename V>
concept multipass_forward_view = forward_view<V> && requires(const V& v, V&& v_mut) {
    { v.forward_iter() } -> multipass_iterator;
    { v_mut.forward_iter() } -> multipass_iterator;
};

template <typename V>
concept multipass_backward_view = backward_view<V> && requires(const V& v, V&& v_mut) {
    { v.backward_iter() } -> multipass_iterator;
    { v_mut.backward_iter() } -> multipass_iterator;
};

/// A bidirectional_view is a view whose elements can be streamed in sequence from either end,
/// and meet in the middle.
template <typename V>
concept bidirectional_view =
    forward_view<V> && backward_view<V> && requires(const V& v, V&& v_mut) {
        requires(std::same_as<iterator_element_type_t<decltype(v.forward_iter())>,
                              iterator_element_type_t<decltype(v.backward_iter())>>);
        requires(std::same_as<iterator_element_type_t<decltype(v_mut.forward_iter())>,
                              iterator_element_type_t<decltype(v_mut.backward_iter())>>);
    };

/// An multipass_bidirectional_view is a view whose iterators can be converted between the forward
/// and backward flavours.
template <typename V>
concept multipass_bidirectional_view = multipass_forward_view<V> && multipass_backward_view<V> &&
                                       bidirectional_view<V> && requires(const V& v, V&& v_mut) {
                                           { v.forward_iter() } -> reversible_iterator;
                                           { v.backward_iter() } -> reversible_iterator;
                                           {
                                               v.forward_iter().invert()
                                           } -> std::same_as<decltype(v.backward_iter())>;
                                           {
                                               v.backward_iter().invert()
                                           } -> std::same_as<decltype(v.forward_iter())>;
                                           { v_mut.forward_iter() } -> reversible_iterator;
                                           { v_mut.backward_iter() } -> reversible_iterator;
                                           {
                                               v_mut.forward_iter().invert()
                                           } -> std::same_as<decltype(v_mut.backward_iter())>;
                                           {
                                               v_mut.backward_iter().invert()
                                           } -> std::same_as<decltype(v_mut.forward_iter())>;
                                       };

/// A emptyness_view is a view that has constant time emptyness check.
template <typename V>
concept emptyness_view = view<V> && requires(const V& v) {
    { v.empty() } -> std::convertible_to<bool>;
};

/// A sized_view is a view that has constant time size.
template <typename V>
concept sized_view = emptyness_view<V> && requires(const V& v) {
    { v.size() } -> std::integral;
};

/// Tag type that represents an infinite size.
struct infinite_t {};

/// An infinite view is a view where iterating all elements takes infinite time.
template <typename V>
concept infinite_view = emptyness_view<V> && requires(const V& v) {
    { v.size() } -> std::same_as<infinite_t>;
};

// Gets the type that this view yields.
template <view T>
struct view_element_type;
template <forward_view T>
struct view_element_type<T> {
    using type = iterator_element_type_t<decltype(std::declval<T>().forward_iter())>;
};
template <backward_view T>
    requires(!forward_view<T>)
struct view_element_type<T> {
    using type = iterator_element_type_t<decltype(std::declval<T>().backward_iter())>;
};
template <view T>
using view_element_type_t = typename view_element_type<T>::type;

struct no_index_type_t {};

namespace impl {
template <view T>
struct view_index_type {
    using type = no_index_type_t;
};
template <typename T>
concept has_index_type = requires { typename std::remove_cvref_t<T>::index_type; };
template <view T>
    requires has_index_type<T>
struct view_index_type<T> {
    using type = typename std::remove_cvref_t<T>::index_type;
};
template <view T>
    requires(!has_index_type<T> && requires { std::declval<T>().size(); })
struct view_index_type<T> {
    using type = decltype(std::declval<T>().size());
};
template <view T>
using view_index_type_t = typename view_index_type<T>::type;
}  // namespace impl

/// A random_access_view is a view that has constant time access of any element by index.  It may be
/// finite or infinite
template <typename V>
concept random_access_view =
    multipass_forward_view<V> &&
    (sized_view<V> || infinite_view<V>)&&std::integral<impl::view_index_type_t<V>> &&
    requires(const V& v, V&& v_mut) {
        {
            v[std::declval<impl::view_index_type_t<V>>()]
        } -> std::same_as<view_element_type_t<const V>>;
        { v.forward_iter() } -> random_access_iterator_with_sentinel<decltype(v.backward_iter())>;
        requires(std::same_as<impl::view_index_type_t<V>,
                              iterator_index_type_t<decltype(v.forward_iter())>>);
        {
            v_mut[std::declval<impl::view_index_type_t<V>>()]
        } -> std::same_as<view_element_type_t<V&&>>;
        {
            v_mut.forward_iter()
        } -> random_access_iterator_with_sentinel<decltype(v_mut.backward_iter())>;
        requires(std::same_as<impl::view_index_type_t<V>,
                              iterator_index_type_t<decltype(v_mut.forward_iter())>>);
    };

/// A finite_random_access_view is a view that has constant time access of any element by index, and
/// is finite.  All random_access_views must also be multipass_bidirectional_view and sized_view.
template <typename V>
concept finite_random_access_view =
    random_access_view<V> && multipass_bidirectional_view<V> && sized_view<V> &&
    requires(const V& v, V&& v_mut) {
        { v.backward_iter() } -> random_access_iterator_with_sentinel<decltype(v.forward_iter())>;
        {
            v_mut.backward_iter()
        } -> random_access_iterator_with_sentinel<decltype(v_mut.forward_iter())>;
    };

template <typename V>
concept infinite_random_access_view = random_access_view<V> && infinite_view<V>;

/// Gets the type to pass to operator[].
template <view T>
struct view_index_type {
    using type = impl::view_index_type_t<T>;
};
template <view T>
using view_index_type_t = typename view_index_type<T>::type;

// --- Adaptors ---
// An adaptor is an object which has a function call operator that takes in a view.  Adaptors
// support being the second argument of operator|.

namespace impl {
template <typename T>
class dummy_forward_random_access_iterator;
template <typename T>
class dummy_backward_random_access_iterator;
template <typename T>
class dummy_forward_random_access_iterator {
   public:
    using index_type = size_t;
    dummy_forward_random_access_iterator();
    T next();
    optional<T> next(const dummy_backward_random_access_iterator<T>&);
    void skip();
    void skip(size_t);
    bool skip(const dummy_backward_random_access_iterator<T>&);
    size_t skip(size_t, const dummy_backward_random_access_iterator<T>&);
    dummy_backward_random_access_iterator<T> invert() const;
};
template <typename T>
class dummy_backward_random_access_iterator {
   public:
    using index_type = size_t;
    dummy_backward_random_access_iterator();
    T next();
    optional<T> next(const dummy_forward_random_access_iterator<T>&);
    void skip();
    void skip(size_t);
    bool skip(const dummy_forward_random_access_iterator<T>&);
    size_t skip(size_t, const dummy_forward_random_access_iterator<T>&);
    dummy_forward_random_access_iterator<T> invert() const;
};
template <typename T>
class dummy_random_access_view {
   public:
    dummy_random_access_view();
    dummy_forward_random_access_iterator<T> forward_iter() const;
    dummy_forward_random_access_iterator<T> forward_iter();
    dummy_backward_random_access_iterator<T> backward_iter() const;
    dummy_backward_random_access_iterator<T> backward_iter();
    bool empty() const;
    size_t size() const;
    T operator[](size_t index) const;
    T operator[](size_t index);
};
static_assert(random_access_view<dummy_random_access_view<int>>);
}  // namespace impl

template <typename A, typename T>
concept adaptor = std::invocable<A, impl::dummy_random_access_view<T>>;

template <view V, adaptor<view_element_type_t<V>> A>
decltype(auto) operator|(V&& v, A&& a) {
    return std::forward<A>(a)(std::forward<V>(v));
}

}  // namespace duality
