// This file is part of https://github.com/btzy/duality
#pragma once

// Stuff that converts a std::forward_list into a view.  When the container is passed by lvalue, it
// will old a reference to the other container.  If passed by rvalue, it will move the container
// into itself.

#include <forward_list>
#include <type_traits>

#include <duality/core_view.hpp>

namespace duality {

namespace impl {

template <typename>
struct is_forward_list : std::false_type {};
template <typename T, typename Allocator>
struct is_forward_list<std::forward_list<T, Allocator>> : std::true_type {};

template <typename C>
concept forward_list_concept = is_forward_list<C>::value;
template <typename C>
concept forward_list_cvref_concept = forward_list_concept<std::remove_cvref_t<C>>;

}  // namespace impl

template <typename C>
    requires impl::forward_list_concept<std::remove_cv_t<C>>
class forward_list_viewifier_nonowning;
template <typename C>
    requires impl::forward_list_concept<C>
class forward_list_viewifier_owning;

namespace impl {

/// This detects if the end iterator doesn't make any references to the list.  It is an optimisation
/// to make the backward iterator contain no fields.  Strictly speaking this is undefined behaviour
/// due to relying on the innards of the standard library, but this works in practice.  This concept
/// is satisfied on libstdc++ and libc++, but not on MSVC (due to checked iterators).
template <typename T>
concept looks_like_end_iterator_is_list_free =
    std::constructible_from<typename T::iterator, std::nullptr_t> &&
    std::constructible_from<typename T::const_iterator, std::nullptr_t>;

template <typename It>
class forward_list_forward_iterator;
template <typename It>
class forward_list_backward_iterator;

class forward_list_backward_iterator_zero_sized {};

// TODO write the iterators

/// @brief Represents a forward iterator of a forward_list.
/// @tparam It The underlying standard forward iterator.
template <typename It>
class forward_list_forward_iterator {
   private:
    friend class forward_list_backward_iterator<It>;
    template <typename C>
        requires forward_list_concept<std::remove_cv_t<C>>
    friend class duality::forward_list_viewifier_nonowning;
    template <typename C>
        requires forward_list_concept<C>
    friend class duality::forward_list_viewifier_owning;
    It it_;
    using element_type = typename It::reference;

    constexpr forward_list_forward_iterator(It it) noexcept : it_(std::move(it)) {}

   public:
    using index_type = no_index_type_t;
    constexpr element_type next() noexcept { return *++it_; }
    constexpr optional<element_type> next(const forward_list_backward_iterator<It>& end) noexcept {
        ++it_;
        if (it_ == end.it_) return nullopt;
        return optional<element_type>(*it_);
    }
    constexpr optional<element_type> next(
        const forward_list_backward_iterator_zero_sized&) noexcept {
        ++it_;
        if (it_ == It(nullptr)) return nullopt;
        return optional<element_type>(*it_);
    }
    constexpr void skip() noexcept { ++it_; }
    constexpr bool skip(const forward_list_backward_iterator<It>& end) noexcept {
        ++it_;
        return it_ != end.it_;
    }
    constexpr bool skip(const forward_list_backward_iterator_zero_sized&) noexcept {
        ++it_;
        return it_ != It(nullptr);
    }
    constexpr forward_list_backward_iterator<It> invert() const noexcept {
        It tmp = it_;
        ++tmp;
        return {tmp};
    }
};

/// @brief Represents a backward iterator of a forward_list.
/// @tparam It The underlying standard forward iterator.
template <typename It>
class forward_list_backward_iterator {
   private:
    friend class forward_list_forward_iterator<It>;
    template <typename C>
        requires forward_list_concept<std::remove_cv_t<C>>
    friend class duality::forward_list_viewifier_nonowning;
    template <typename C>
        requires forward_list_concept<C>
    friend class duality::forward_list_viewifier_owning;
    It it_;

    constexpr forward_list_backward_iterator(It it) noexcept : it_(std::move(it)) {}
};

}  // namespace impl

/// @brief Represents a view that references a forward_list.
/// @tparam C The container type.  Should be a forward_list or const forward_list.
template <typename C>
    requires impl::forward_list_concept<std::remove_cv_t<C>>
class forward_list_viewifier_nonowning {
   private:
    using iterator = decltype(std::declval<C>().begin());
    [[no_unique_address]] iterator before_begin_;
    [[no_unique_address]] iterator end_;

   public:
    template <typename C2>
    constexpr forward_list_viewifier_nonowning(C2& c) noexcept
        : before_begin_(c.before_begin()), end_(c.end()) {}
    constexpr auto forward_iter() const noexcept {
        return impl::forward_list_forward_iterator<iterator>(before_begin_);
    }
    constexpr auto backward_iter() const noexcept {
        return impl::forward_list_backward_iterator<iterator>(end_);
    }
    constexpr bool empty() const noexcept {
        iterator tmp = before_begin_;
        ++tmp;
        return tmp == end_;
    }
};

/// @brief Specialization where we don't need to save the end iterator.
/// @tparam C The container type.
template <typename C>
    requires impl::forward_list_concept<std::remove_cv_t<C>> &&
             impl::looks_like_end_iterator_is_list_free<std::remove_cv_t<C>>
class forward_list_viewifier_nonowning<C> {
   private:
    using iterator = decltype(std::declval<C>().begin());
    [[no_unique_address]] iterator before_begin_;

   public:
    template <typename C2>
    constexpr forward_list_viewifier_nonowning(C2& c) noexcept : before_begin_(c.before_begin()) {}
    constexpr auto forward_iter() const noexcept {
        return impl::forward_list_forward_iterator<iterator>(before_begin_);
    }
    constexpr auto backward_iter() const noexcept {
        return impl::forward_list_backward_iterator_zero_sized();
    }
    constexpr bool empty() const noexcept {
        iterator tmp = before_begin_;
        ++tmp;
        return tmp == iterator(nullptr);
    }
};

template <typename C>
    requires impl::forward_list_concept<std::remove_cv_t<C>>
forward_list_viewifier_nonowning(C&) -> forward_list_viewifier_nonowning<C>;

/// @brief Represents a view that contains a forward_list.
/// @tparam C The container type.  Should be a forward_list or const forward_list.
template <typename C>
    requires impl::forward_list_concept<C>
class forward_list_viewifier_owning {
   private:
    C container_;

   public:
    template <typename C2>
    constexpr forward_list_viewifier_owning(C2&& c) noexcept(noexcept(C(std::forward<C2>(c))))
        : container_(std::forward<C2>(c)) {}
    constexpr auto forward_iter() noexcept {
        return impl::forward_list_forward_iterator<typename C::iterator>(container_.before_begin());
    }
    constexpr auto forward_iter() const noexcept {
        return impl::forward_list_forward_iterator<typename C::const_iterator>(
            container_.before_begin());
    }
    constexpr auto backward_iter() noexcept
        requires(!impl::looks_like_end_iterator_is_list_free<C>)
    {
        return impl::forward_list_backward_iterator<typename C::iterator>(container_.end());
    }
    constexpr auto backward_iter() const noexcept
        requires(!impl::looks_like_end_iterator_is_list_free<C>)
    {
        return impl::forward_list_backward_iterator<typename C::const_iterator>(container_.end());
    }
    constexpr auto backward_iter() noexcept
        requires impl::looks_like_end_iterator_is_list_free<C>
    {
        return impl::forward_list_backward_iterator_zero_sized();
    }
    constexpr auto backward_iter() const noexcept
        requires impl::looks_like_end_iterator_is_list_free<C>
    {
        return impl::forward_list_backward_iterator_zero_sized();
    }
    constexpr bool empty() const noexcept { return container_.empty(); }
};

template <typename C>
    requires impl::forward_list_concept<C>
forward_list_viewifier_owning(C&&) -> forward_list_viewifier_owning<std::remove_cvref_t<C>>;

/// @brief This function returns a wrapper for the given forward_list (by lvalue reference).
/// @tparam T A forward_list type.
/// @param v The forward_list.
/// @return The wrapper.
template <impl::forward_list_cvref_concept T>
constexpr auto viewify(T& t) {
    return forward_list_viewifier_nonowning(t);
}

template <
    impl::forward_list_cvref_concept T,
    adaptor<view_element_type_t<decltype(forward_list_viewifier_nonowning(std::declval<T&>()))>> A>
decltype(auto) operator|(T& t, A&& a) {
    return std::forward<A>(a)(forward_list_viewifier_nonowning(t));
}

/// @brief This function returns a wrapper for the given forward_list (by move).
/// @tparam T A forward_list type.
/// @param v The forward_list.
/// @return The wrapper.
template <impl::forward_list_cvref_concept T>
constexpr auto viewify(T&& t) {
    return forward_list_viewifier_owning(std::move(t));
}

template <
    impl::forward_list_cvref_concept T,
    adaptor<view_element_type_t<decltype(forward_list_viewifier_owning(std::declval<T&&>()))>> A>
decltype(auto) operator|(T&& t, A&& a) {
    return std::forward<A>(a)(forward_list_viewifier_owning(std::forward<T>(t)));
}

}  // namespace duality
