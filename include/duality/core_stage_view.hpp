// This file is part of https://github.com/btzy/duality
#pragma once

// This file contains all the core stage_view concepts.  A stage_view is a class that you can obtain
// a view from by calling stage().  This allows the view created from a stage_view to have scratch
// space.  Typically, views created from a stage_view are not multipass.

#include <concepts>
#include <type_traits>

#include <duality/core_iterator.hpp>
#include <duality/core_view.hpp>

namespace duality {

template <typename S>
concept stage_view = std::move_constructible<std::remove_cvref_t<S>> && requires(S&& s) {
    { s.stage() } -> view;
};

template <stage_view S>
struct stage_type {
    using type = decltype(std::declval<S>().stage());
};
template <stage_view S>
using stage_type_t = typename stage_type<S>::type;

template <stage_view S, adaptor<view_element_type_t<stage_type_t<S>>> A>
class adapted_stage_view {
   private:
    S s_;
    A a_;

   public:
    template <stage_view S2, adaptor<view_element_type_t<stage_type_t<S2>>> A2>
    constexpr adapted_stage_view(S2&& s, A2&& a)
        : s_(std::forward<S2>(s)), a_(std::forward<A2>(a)) {}
    constexpr decltype(auto) stage() const& { return a_(s_.stage()); }
    constexpr decltype(auto) stage() && { return std::forward<A>(a_)(std::forward<S>(s_).stage()); }
};

template <stage_view S2, adaptor<view_element_type_t<stage_type_t<S2>>> A2>
adapted_stage_view(S2&& s, A2&& a) -> adapted_stage_view<S2, A2>;

template <stage_view S, adaptor<view_element_type_t<decltype(std::declval<S>().stage())>> A>
constexpr decltype(auto) operator|(S&& s, A&& a) {
    adapted_stage_view(std::forward<S>(s), std::forward<A>(a));
}

template <typename S>
constexpr decltype(auto) stage_all(S&& s) {
    return std::forward<S>(s);
}

template <stage_view S>
constexpr decltype(auto) stage_all(S&& s) {
    return stage_all(std::forward<S>(s).stage());
}

template <typename S>
struct stage_all_type {
    using type = decltype(stage_all(std::declval<S>()));
};
template <typename S>
using stage_all_type_t = typename stage_all_type<S>::type;

}  // namespace duality
