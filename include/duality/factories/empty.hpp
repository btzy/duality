// This file is part of https://github.com/btzy/duality
#pragma once

// A view that contains a single element.

#include <concepts>
#include <utility>

#include <duality/builtin_assume.hpp>
#include <duality/core_view.hpp>

namespace duality {

template <typename T>
class empty_view;

namespace impl {

/// @brief Iterator for empty_view (both forward and backward).
/// @tparam T The type of yielded elements (can be a reference).
template <typename T>
class empty_iterator {
   private:
    using element_type = T;
    template <typename T2>
    friend class duality::empty_view;

   public:
    using index_type = size_t;
    UNREACHABLE_RETURN_BEGIN
#if !defined(__GNUC__) || defined(__clang__)
    // GCC doesn't like constexpr functions to not have a return statement, but we can't utter a
    // return statement because that will require T to be default constructable.
    constexpr
#endif
        element_type
        next() noexcept {
        // It is always UB to call this function, since we're definitely already at the end.
        builtin_unreachable();
    }
    UNREACHABLE_RETURN_END
    constexpr optional<element_type> next(const empty_iterator<T>&) noexcept { return nullopt; }
    [[noreturn]] constexpr void skip() noexcept {
        // It is always UB to call this function, since we're definitely already at the end.
        builtin_unreachable();
    }
    constexpr bool skip(const empty_iterator<T>&) noexcept { return false; }
    constexpr void skip(size_t) noexcept {
        // Do nothing.  If count==0, then this is a well-defined no-op.  Otherwise, this is UB.
    }
    constexpr size_t skip(size_t, const empty_iterator<T>&) noexcept {
        // If count==0, then this is a no-op and always succeeds.  Otherwise, we will hit the end
        // and so we return false.
        return 0;
    }
    constexpr empty_iterator<T> invert() const noexcept {
        // Inverting the iterator yields a copy of itself, since all empty_iterators are sorta
        // equivalent.
        return {};
    }
};

}  // namespace impl

/// @brief View containing no elements.
/// @tparam T The type of yielded elements (can be a reference).
template <typename T>
class empty_view {
   public:
    using index_type = size_t;
    constexpr empty_view() noexcept = default;
    constexpr impl::empty_iterator<T> forward_iter() const noexcept { return {}; }
    constexpr impl::empty_iterator<T> backward_iter() const noexcept { return {}; }
    constexpr static bool empty() noexcept { return true; }
    constexpr static size_t size() noexcept { return 0; }
    UNREACHABLE_RETURN_BEGIN
#if !defined(__GNUC__) || defined(__clang__)
    // GCC doesn't like constexpr functions to not have a return statement, but we can't utter a
    // return statement because that will require T to be default constructable.
    constexpr
#endif
        T
        operator[](index_type) const noexcept {
        // Since there are no elements, this access is always out of bounds, and so it's always UB.
        impl::builtin_unreachable();
    }
    UNREACHABLE_RETURN_END
};

namespace factories {

template <typename T>
constexpr empty_view<T> empty() noexcept {
    return {};
}

}  // namespace factories

}  // namespace duality
