// This file is part of https://github.com/btzy/duality
#pragma once

// Stuff that converts a contiguous container into a view.  When the container is passed by lvalue,
// it will old a reference to the other container.  If passed by rvalue, it will move the container
// into itself.

#include <iterator>
#include <type_traits>

#include <duality/core_view.hpp>

namespace duality {

namespace impl {

template <typename T>
concept contiguous_container = requires(T& t) {
    { std::begin(t) } -> std::contiguous_iterator;
    { std::end(t) } -> std::same_as<decltype(std::begin(t))>;
};
}  // namespace impl

template <typename T>
    requires(!std::is_reference_v<T>)
class contiguous_viewifier_nonowning;
template <impl::contiguous_container T>
    requires(!std::is_reference_v<T>)
class contiguous_viewifier_owning;

namespace impl {

template <typename T>
    requires(!std::is_reference_v<T>)
class contiguous_forward_iterator;
template <typename T>
    requires(!std::is_reference_v<T>)
class contiguous_backward_iterator;

/// @brief Represents a forward iterator of a continguous range.
/// @tparam T The type of elements in the range.
template <typename T>
    requires(!std::is_reference_v<T>)
class contiguous_forward_iterator {
   private:
    friend class contiguous_backward_iterator<T>;
    friend class contiguous_viewifier_nonowning<T>;
    template <contiguous_container T2>
        requires(!std::is_reference_v<T2>)
    friend class duality::contiguous_viewifier_owning;
    T* ptr_;

    constexpr contiguous_forward_iterator(T* ptr) noexcept : ptr_(ptr) {}

   public:
    using index_type = size_t;
    constexpr T& next() noexcept { return *ptr_++; }
    constexpr optional<T&> next(const contiguous_backward_iterator<T>& end) noexcept {
        if (ptr_ == end.ptr_) return nullopt;
        return optional<T&>(*ptr_++);
    }
    constexpr void skip() noexcept { ++ptr_; }
    constexpr void skip(size_t count) noexcept { ptr_ += count; }
    constexpr bool skip(const contiguous_backward_iterator<T>& end) noexcept {
        if (ptr_ != end.ptr_) {
            ++ptr_;
            return true;
        }
        return false;
    }
    constexpr size_t skip(size_t count, const contiguous_backward_iterator<T>& end) noexcept {
        size_t diff = static_cast<size_t>(end.ptr_ - ptr_);
        if (diff >= count) {
            ptr_ += count;
            return count;
        }
        ptr_ = end.ptr_;
        return diff;
    }
    constexpr size_t skip(infinite_t, const contiguous_backward_iterator<T>& end) noexcept {
        size_t diff = static_cast<size_t>(end.ptr_ - ptr_);
        ptr_ = end.ptr_;
        return diff;
    }
    constexpr contiguous_backward_iterator<T> invert() const noexcept { return {ptr_}; }
};

/// @brief Represents a backward iterator of a continguous range.
/// @tparam T The type of elements in the range.
template <typename T>
    requires(!std::is_reference_v<T>)
class contiguous_backward_iterator {
   private:
    friend class contiguous_forward_iterator<T>;
    friend class contiguous_viewifier_nonowning<T>;
    template <contiguous_container T2>
        requires(!std::is_reference_v<T2>)
    friend class duality::contiguous_viewifier_owning;
    T* ptr_;

    constexpr contiguous_backward_iterator(T* ptr) noexcept : ptr_(ptr) {}

   public:
    using index_type = size_t;
    constexpr T& next() noexcept { return *--ptr_; }
    constexpr optional<T&> next(const contiguous_forward_iterator<T>& end) noexcept {
        if (ptr_ == end.ptr_) return nullopt;
        return optional<T&>(*--ptr_);
    }
    constexpr void skip() noexcept { --ptr_; }
    constexpr void skip(size_t count) noexcept { ptr_ -= count; }
    constexpr bool skip(const contiguous_forward_iterator<T>& end) noexcept {
        if (ptr_ != end.ptr_) {
            --ptr_;
            return true;
        }
        return false;
    }
    constexpr size_t skip(size_t count, const contiguous_forward_iterator<T>& end) noexcept {
        size_t diff = static_cast<size_t>(ptr_ - end.ptr_);
        if (diff >= count) {
            ptr_ -= count;
            return count;
        }
        ptr_ = end.ptr_;
        return diff;
    }
    constexpr size_t skip(infinite_t, const contiguous_forward_iterator<T>& end) noexcept {
        size_t diff = static_cast<size_t>(ptr_ - end.ptr_);
        ptr_ = end.ptr_;
        return diff;
    }
    constexpr contiguous_forward_iterator<T> invert() const noexcept { return {ptr_}; }
};
}  // namespace impl

/// @brief Represents a view that references a continguous range.
/// @tparam T A non-reference type (possibly const) representing the type of elements in the view.
template <typename T>
    requires(!std::is_reference_v<T>)
class contiguous_viewifier_nonowning {
   private:
    T* begin_;
    T* end_;

    constexpr contiguous_viewifier_nonowning(T* begin, T* end) noexcept
        : begin_(begin), end_(end) {}

   public:
    template <impl::contiguous_container T2>
    constexpr contiguous_viewifier_nonowning(T2& t) noexcept(
        noexcept(std::to_address(std::begin(t))) && noexcept(std::to_address(std::end(t))))
        : contiguous_viewifier_nonowning(std::to_address(std::begin(t)),
                                         std::to_address(std::end(t))) {}
    constexpr auto forward_iter() const noexcept {
        return impl::contiguous_forward_iterator<T>(begin_);
    }
    constexpr auto backward_iter() const noexcept {
        return impl::contiguous_backward_iterator<T>(end_);
    }
    constexpr bool empty() const noexcept { return begin_ == end_; }
    constexpr size_t size() const noexcept { return end_ - begin_; }
};

template <impl::contiguous_container T>
contiguous_viewifier_nonowning(T& t) -> contiguous_viewifier_nonowning<typename T::value_type>;

/// @brief Represents view that contains a continguous range.
/// @tparam T The contiguous container being wrapped.
template <impl::contiguous_container T>
    requires(!std::is_reference_v<T>)
class contiguous_viewifier_owning {
   private:
    T container_;

   public:
    template <impl::contiguous_container T2>
    constexpr contiguous_viewifier_owning(T2&& t) noexcept(noexcept(T(std::forward<T2>(t))))
        : container_(std::forward<T2>(t)) {}
    constexpr auto forward_iter() noexcept {
        return impl::contiguous_forward_iterator<typename T::value_type>(
            std::to_address(std::begin(container_)));
    }
    constexpr auto forward_iter() const noexcept {
        return impl::contiguous_forward_iterator<const typename T::value_type>(
            std::to_address(std::begin(container_)));
    }
    constexpr auto backward_iter() noexcept {
        return impl::contiguous_backward_iterator<typename T::value_type>(
            std::to_address(std::end(container_)));
    }
    constexpr auto backward_iter() const noexcept {
        return impl::contiguous_backward_iterator<const typename T::value_type>(
            std::to_address(std::end(container_)));
    }
    constexpr bool empty() const noexcept { return std::empty(container_); }
    constexpr size_t size() const noexcept { return std::size(container_); }
};

template <impl::contiguous_container T>
contiguous_viewifier_owning(T&& t) -> contiguous_viewifier_owning<std::remove_cvref_t<T>>;

/// @brief This function returns a wrapper for the given contiguous container (by lvalue reference).
/// @tparam T A contiguous container type.
/// @param v The container.
/// @return The wrapper.
template <impl::contiguous_container T>
constexpr auto viewify(T& t) {
    return contiguous_viewifier_nonowning(t);
}

template <
    impl::contiguous_container T,
    adaptor<view_element_type_t<decltype(contiguous_viewifier_nonowning(std::declval<T&>()))>> A>
decltype(auto) operator|(T& t, A&& a) {
    return std::forward<A>(a)(contiguous_viewifier_nonowning(t));
}

/// @brief This function returns a wrapper for the given contiguous container (by move).
/// @tparam T A contiguous container type.
/// @param v The container.
/// @return The wrapper.
template <impl::contiguous_container T>
constexpr auto viewify(T&& t) {
    return contiguous_viewifier_owning(std::move(t));
}

template <
    impl::contiguous_container T,
    adaptor<view_element_type_t<decltype(contiguous_viewifier_owning(std::declval<T&&>()))>> A>
decltype(auto) operator|(T&& t, A&& a) {
    return std::forward<A>(a)(contiguous_viewifier_owning(std::forward<T>(t)));
}

}  // namespace duality
