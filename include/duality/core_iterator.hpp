// This file is part of https://github.com/btzy/duality
#pragma once

// This file contains all the core iterator concepts.

#include <concepts>
#include <utility>

#include <duality/optional.hpp>

namespace duality {

/// Tag type that represents an infinite size.
struct infinite_t {};
constexpr inline infinite_t infinite;

namespace impl {
template <typename T>
concept non_void = !std::same_as<T, void>;
template <typename I, typename S>
using distance_t = decltype(std::declval<I>().skip(infinite_t{}, std::declval<const S>()));
}  // namespace impl

/// Concept for an iterator.
template <typename I>
concept iterator = std::movable<I> && requires(I&& i) {
    { i.next() } -> impl::non_void;
    i.skip();
};

/// Concept for the sentinel of an iterator.
/// If i is produced from an invert() without any other next or skip operations on i, then either of
/// the following needs to be true before calling this flavour of next or skip:
/// 1) s is _strictly after_ i, or
/// 2) s is _at the same location_ as i, and s is the original iterator that was inverted to get i.
///
/// Here, _at the same location_ means that calling any flavours of next and skip will yield the
/// same elements on both iterators.
///
/// After the iterator fails to produce a value (i.e. next(s) returns nullopt or skip(s) returns
/// false), this iterator will be in an unspecified state, suitable only for reassignment or
/// destruction.
template <typename S, typename I>
concept sentinel_for = requires(const S& s, I&& i) {
    { i.next(s) } -> std::same_as<optional<decltype(i.next())>>;
    { i.skip(s) } -> std::same_as<bool>;
};

/// Concept for a multipass iterator (i.e. one that can be inverted, but the inverse might only be a
/// sentinel).
template <typename I>
concept multipass_iterator = iterator<I> && std::copyable<I> && requires(I&& i) {
    { i.invert() } -> sentinel_for<I>;
};

/// Concept for an reversible iterator (i.e. one whose inverse is also an iterator).
template <typename I>
concept reversible_iterator = multipass_iterator<I> && requires(const I& i) {
    { i.invert() } -> multipass_iterator;
    { i.invert().invert() } -> std::same_as<I>;
};

/// Concept for a random access iterator (i.e. one that can skip any number of elements in constant
/// time).
///
/// A skip(index, s) operation that returns a value less than index will be an iterator to the end
/// of the view.  If this iterator is multipass, then inverting the iterator will yield a sentinel
/// equivalent to s.
template <typename I>
concept random_access_iterator =
    reversible_iterator<I> && reversible_iterator<decltype(std::declval<I>().invert())> &&
    sentinel_for<I, decltype(std::declval<I>().invert())> &&
    std::integral<typename I::index_type> && requires(I&& i, typename I::index_type index) {
        i.skip(index);
        {
            i.skip(index, i.invert())
        } -> std::same_as<typename I::index_type>;  // returns the amount skipped
        {
            i.skip(infinite_t{}, i.invert())
        } -> std::same_as<typename I::index_type>;  // returns the amount skipped
        std::declval<decltype(i.invert())>().skip(index);
        std::declval<decltype(i.invert())>().skip(index, static_cast<const I&>(i));
        std::declval<decltype(i.invert())>().skip(static_cast<const I&>(i));
    };

/// Concept for a random access iterator (i.e. one that can skip any number of elements in constant
/// time).
template <typename I, typename S>
concept random_access_iterator_with_sentinel =
    random_access_iterator<I> && sentinel_for<S, I> &&
    requires(I&& i, const S& s, typename I::index_type index) {
        { i.skip(index, s) } -> std::same_as<typename I::index_type>;  // returns the amount skipped
    } &&
    (std::same_as<impl::distance_t<I, S>, typename I::index_type> ||
     std::same_as<impl::distance_t<I, S>, infinite_t>);

/// Gets the type that this iterator yields.
template <iterator I>
struct iterator_element_type {
    using type = decltype(std::declval<I>().next());
};
template <iterator I>
using iterator_element_type_t = typename iterator_element_type<I>::type;

/// Gets the type used for skipping elements.
template <iterator I>
struct iterator_index_type {
    using type = typename I::index_type;
};
template <iterator I>
using iterator_index_type_t = typename iterator_index_type<I>::type;

// /// Gets the type that this iterator inverts to.
// template <reversible_iterator I>
// struct iterator_inverted_type {
//     using type = decltype(std::declval<I>().invert());
// };
// template <reversible_iterator I>
// using iterator_inverted_type_t = typename iterator_inverted_type<I>::type;

}  // namespace duality
