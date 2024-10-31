// This file is part of https://github.com/btzy/duality
#pragma once

// Operation that consumes a view into a container.  There is a default but perhaps suboptimal
// implementation that works with most containers, but containers can implement a constructor from a
// view to get optimal performance.

#include <concepts>
#include <forward_list>
#include <map>
#include <ranges>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <duality/builtin_assume.hpp>
#include <duality/core_view.hpp>
#include <duality/views/as_input_range.hpp>

namespace duality {

struct from_view_t {};
constexpr inline from_view_t from_view{};

namespace impl {

template <typename C, typename V, typename... Args>
concept constructible_from_range_with_reserve =
    sized_view<V> && requires(C& c, V&& v, Args&&... args) {
        { C(std::forward<Args>(args)...) } -> std::same_as<C>;
        c.reserve(v.size());
        c.size();
        c.capacity();
        c.append_range(as_input_range_view(wrapping_construct, std::forward<V>(v)));
    };

template <typename C, typename T>
concept container_appendable = requires(C& c, T&& t) {
    requires(
        requires { c.emplace_back(std::forward<T>(t)); } ||
        requires { c.push_back(std::forward<T>(t)); } ||
        requires { c.emplace(c.end(), std::forward<T>(t)); } ||
        requires { c.insert(c.end(), std::forward<T>(t)); });
};

template <typename C, typename T>
    requires container_appendable<C, T>
inline void container_append(C& c, T&& t) {
    if constexpr (requires { c.emplace_back(std::declval<T>()); })
        c.emplace_back(std::forward<T>(t));
    else if constexpr (requires { c.push_back(std::declval<T>()); })
        c.push_back(std::forward<T>(t));
    else if constexpr (requires { c.emplace(c.end(), std::declval<T>()); })
        c.emplace(c.end(), std::forward<T>(t));
    else
        c.insert(c.end(), std::forward<T>(t));
}

template <typename C, typename V, typename... Args>
concept constructible_from_container_appender =
    container_appendable<C, views::element_type_t<V>> && requires(C& c, V&& v, Args&&... args) {
        { C(std::forward<Args>(args)...) } -> std::same_as<C>;
    };

template <typename C, typename V, typename... Args>
concept constructible_from_container_appender_with_size =
    constructible_from_container_appender<C, V, Args...> && sized_view<V> &&
    requires(C& c) { c.size(); };

template <typename C, typename V, typename... Args>
concept constructible_from_container_appender_with_reserve =
    constructible_from_container_appender_with_size<C, V, Args...> && requires(C& c, V&& v) {
        c.reserve(v.size());
        c.capacity();
    };

}  // namespace impl

/// @brief Converts a view to the given container.  This is a best effort conversion function that
/// should work with most containers.  If you are implementing a new container, you should add a
/// constructor from `from_view_t` and the view (i.e. the first branch in the constexpr-if statement
/// in this function).
/// @tparam C Type of the target container.
/// @tparam ...Args Types of the extra arguments for the container, e.g. for the allocator.
/// @tparam V Type of the view.
/// @param v The view.
/// @param ...args The extra args, e.g. the allocator.
/// @return The container containing the elements from the view.
template <typename C, forward_view V, typename... Args>
constexpr C to_container(V&& v, Args&&... args) {
    if constexpr (std::constructible_from<C, from_view_t, V, Args...>) {
        // container supports constructing from view
        return C(from_view, std::forward<V>(v), std::forward<Args>(args)...);
    } else if constexpr (impl::constructible_from_range_with_reserve<C, V, Args...>) {
        // reserve and append range, with some builtins for hopefully better optimization
        C res(std::forward<Args>(args)...);
        const auto size = v.size();
        res.reserve(size);
        const auto original_capacity = res.capacity();
        res.append_range(as_input_range_view(wrapping_construct, std::forward<V>(v)));
        impl::builtin_assume(size == res.size());
        impl::builtin_assume(original_capacity == res.capacity());
        return res;
    } else if constexpr (impl::constructible_from_container_appender_with_reserve<C, V, Args...>) {
        // reserve and append elements, with some builtins for hopefully better optimization
        C res(std::forward<Args>(args)...);
        const auto size = v.size();
        res.reserve(size);
        const auto original_capacity = res.capacity();
        auto&& stream = view_ops::stream(v);
        for (size_t i = 0; i != size; ++i) {
            impl::container_append(res, *stream.next());
            impl::builtin_assume(original_capacity == res.capacity());
        }
        impl::builtin_assume(size == res.size());
        impl::builtin_assume(original_capacity == res.capacity());
        return res;
#if defined(__cpp_lib_ranges_to_container) && __cpp_lib_ranges_to_container >= 202202L
    } else if constexpr (std::constructible_from<C,
                                                 std::from_range_t,
                                                 decltype(as_input_range_view(wrapping_construct,
                                                                              std::declval<V&&>())),
                                                 Args...>) {
        // container supports constructing from C++20 ranges
        return C(std::from_range,
                 as_input_range_view(wrapping_construct, std::forward<V>(v)),
                 std::forward<Args>(args)...);
#endif
    } else if constexpr (impl::constructible_from_container_appender_with_size<C, V, Args...>) {
        // append elements (without reserve), with some builtins for hopefully better optimization
        C res(std::forward<Args>(args)...);
        const auto size = view_ops::size(v);
        auto&& stream = view_ops::stream(v);
        for (size_t i = 0; i != size; ++i) {
            impl::container_append(res, *stream.next());
        }
        impl::builtin_assume(size == res.size());
        return res;
    } else if constexpr (impl::constructible_from_container_appender<C, V, Args...>) {
        // append elements from an unsized view
        C res(std::forward<Args>(args)...);
        auto&& stream = view_ops::stream(v);
        while (auto x = stream.next()) {
            impl::container_append(res, *std::move(x));
        }
        return res;
    } else {
        static_assert(!std::same_as<C, C>,
                      "Cannot consume view into container, you need to add a constructor from a "
                      "view (with from_view_t tag).");
    }
}

namespace impl {
template <template <typename...> typename C, typename T>
struct is_specialization : std::false_type {};
template <template <typename...> typename C, typename... Args>
struct is_specialization<C, C<Args...>> : std::true_type {};
template <template <typename...> typename C, typename T>
constexpr inline bool is_specialization_v = is_specialization<C, T>::value;
}  // namespace impl

/// @brief Specialization for forward_list because it does not have
/// push_back/emplace_back/insert/emplace.
template <typename C, forward_view V, typename... Args>
    requires impl::is_specialization_v<std::forward_list, C> &&
             std::constructible_from<typename C::value_type, views::element_type_t<V>>
constexpr C to_container(V&& v, Args&&... args) {
    C res(std::forward<Args>(args)...);
    auto it = res.before_begin();
    auto&& stream = view_ops::stream(v);
    while (auto x = stream.next()) {
        it = res.insert_after(it, *std::move(x));
    }
    return res;
}

/// @brief Specialization for set because it does not have
/// push_back/emplace_back/insert/emplace.
template <typename C, forward_view V, typename... Args>
    requires impl::is_specialization_v<std::set, C> &&
             std::constructible_from<typename C::value_type, views::element_type_t<V>>
constexpr C to_container(V&& v, Args&&... args) {
    C res(std::forward<Args>(args)...);
    auto&& stream = view_ops::stream(v);
    while (auto x = stream.next()) {
        res.insert(*std::move(x));
    }
    return res;
}

/// @brief Specialization for map because it does not have
/// push_back/emplace_back/insert/emplace.
template <typename C, forward_view V, typename... Args>
    requires impl::is_specialization_v<std::map, C> &&
             std::constructible_from<typename C::value_type, views::element_type_t<V>>
constexpr C to_container(V&& v, Args&&... args) {
    C res(std::forward<Args>(args)...);
    auto&& stream = view_ops::stream(v);
    while (auto x = stream.next()) {
        res.insert(*std::move(x));
    }
    return res;
}

/// @brief Specialization for unordered_set because it does not have
/// push_back/emplace_back/insert/emplace.
template <typename C, forward_view V, typename... Args>
    requires impl::is_specialization_v<std::unordered_set, C> &&
             std::constructible_from<typename C::value_type, views::element_type_t<V>>
constexpr C to_container(V&& v, Args&&... args) {
    C res(std::forward<Args>(args)...);
    auto&& stream = view_ops::stream(v);
    while (auto x = stream.next()) {
        res.insert(*std::move(x));
    }
    return res;
}

/// @brief Specialization for unordered_map because it does not have
/// push_back/emplace_back/insert/emplace.
template <typename C, forward_view V, typename... Args>
    requires impl::is_specialization_v<std::unordered_map, C> &&
             std::constructible_from<typename C::value_type, views::element_type_t<V>>
constexpr C to_container(V&& v, Args&&... args) {
    C res(std::forward<Args>(args)...);
    auto&& stream = view_ops::stream(v);
    while (auto x = stream.next()) {
        res.insert(*std::move(x));
    }
    return res;
}

namespace impl {

template <typename C, typename... Args>
struct to_adaptor {
    template <forward_view V>
    constexpr C operator()(V&& v) const& {
        return std::apply([&](auto&&... xs) { return to_container<C>(std::forward<V>(v), xs...); },
                          args);
    }
    template <forward_view V>
    constexpr C operator()(V&& v) && {
        return std::apply(
            [&](auto&&... xs) {
                return to_container<C>(std::forward<V>(v), std::forward<Args>(xs)...);
            },
            std::move(args));
    }
    [[no_unique_address]] std::tuple<Args&&...> args;
};

}  // namespace impl

namespace operations {
template <typename C, forward_view V, typename... Args>
constexpr C to(V&& v, Args&&... args) {
    return to_container<C>(std::forward<V>(v), std::forward<Args>(args)...);
}
/// @brief Returns an adaptor that converts the view into a contaier.
/// @tparam C The type of container to convert into.
/// @tparam ...Args Type of extra arguments for the container, e.g. allocator.
/// @param ...args Extra arguments for the container, e.g. allocator.
/// @return The container containing the elements from the view.
template <typename C, typename... Args>
constexpr auto to(Args&&... args) {
    return impl::to_adaptor<C, Args...>{std::forward_as_tuple(std::forward<Args>(args)...)};
}

}  // namespace operations

}  // namespace duality
