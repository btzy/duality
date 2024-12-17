// This file is part of https://github.com/btzy/duality
#pragma once

#include <array>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <duality/builtin_assume.hpp>
#include <duality/core_view.hpp>

// TODO: requires new view reification

/// Implementation for views::concat.  Concatenates one or more views together.
///
/// E.g.:
/// concat({1, 3, 5}, {2, 4, 6, 4, 1}) -> {1, 3, 5, 2, 4, 6, 4, 1}
/// concat({1, 3, 5}) -> {1, 3, 5}   (iterator-preserving in this case)
///
/// Let R be the list of views from the first view up to the first infinite view, if such exists,
/// otherwise the first view up to the last view.
///
/// For each concept C in {forward_view, backward_view, multipass_forward_view,
/// multipass_backward_view, bidirectional_view, multipass_bidirectional_view, emptyness_view,
/// sized_view, random_access_bidirectional_view}, the resultant view
/// satisfies C if all the views in R satisfies C.
///
/// The resultant view satisfies infinite_view if at least one view in R satisfies infinite_view.
///
/// concat_view requires that either all views satisfy forward_view or all views satisfy
/// backward_view, since the returned view needs to satisfy view.

namespace duality {

namespace impl {

template <typename T, typename... Ts>
concept same_with_all = (... && std::same_as<T, Ts>);

template <typename... Ts>
concept all_same = (... && same_with_all<Ts, Ts...>);

template <view...>
struct all_finite_except_last;
template <view V>
struct all_finite_except_last<V> : std::true_type {};
template <view V1, view V2, view... Vs>
struct all_finite_except_last<V1, V2, Vs...> : all_finite_except_last<V2, Vs...> {};
template <infinite_view V1, view V2, view... Vs>
struct all_finite_except_last<V1, V2, Vs...> : std::false_type {};
template <view... Vs>
constexpr bool all_finite_except_last_v = all_finite_except_last<Vs...>::value;

// Shim until we can get C++26 subsumption of parameter packs.
template <typename... Vs>
concept concatable_views =
    ((... && view<Vs>)) && (sizeof...(Vs) >= 1) && all_same<view_element_type_t<Vs>...> &&
    all_finite_except_last_v<Vs...> && ((... && forward_view<Vs>) || (... && backward_view<Vs>));
template <typename V>
concept single_concatable_views = concatable_views<V>;
template <typename... Vs>
concept multi_concatable_views = concatable_views<Vs...> && (sizeof...(Vs) >= 2);

}  // namespace impl

template <typename... Vs>
    requires impl::concatable_views<Vs...>
class concat_view;

namespace impl {

template <typename Ret, typename F, size_t I, typename Variant, typename... Args>
constexpr Ret visit_with_index_jump_table_hook(Variant&& v, Args&&... args) {
    builtin_assume(I == v.index());
    return F{}.template operator()<I>(std::get<I>(std::forward<Variant>(v)),
                                      std::forward<Args>(args)...);
}

template <typename F, typename IndexSeq, typename Variant, typename... Args>
struct visit_with_index_jump_table;
template <typename F, size_t... Is, typename Variant, typename... Args>
struct visit_with_index_jump_table<F, std::index_sequence<Is...>, Variant, Args...> {
    static_assert(all_same<decltype(std::declval<F>().template operator()<Is>(
                      std::get<Is>(std::declval<Variant>()),
                      std::declval<Args>()...))...>);
    using type =
        decltype(std::declval<F>().template operator()<0>(std::get<0>(std::declval<Variant>()),
                                                          std::declval<Args>()...));
    constexpr static std::array<type (*)(Variant&&, Args&&...), sizeof...(Is)> value = {
        &visit_with_index_jump_table_hook<type, F, Is, Variant, Args...>...};
};

template <typename F, typename Variant, typename... Args>
    requires(sizeof(std::remove_cvref_t<F>) ==
             1)  // really we want to check for statelessness, but it is difficult to write it
constexpr decltype(auto) visit_with_index(F&&, Variant&& v, Args&&... args) {
    const auto index = v.index();
    return (*visit_with_index_jump_table<
            F,
            std::make_index_sequence<std::variant_size_v<std::remove_cvref_t<Variant>>>,
            Variant,
            Args...>::value[index])(std::forward<Variant>(v), std::forward<Args>(args)...);
}

// Compatibility fill until we have C++26, due to https://stackoverflow.com/q/79178032/1021959
template <typename... Vs>
concept views = (... && view<Vs>);
template <typename... Vs>
concept random_access_views = views<Vs...> && (... && random_access_bidirectional_view<Vs>);
template <typename... Vs>
concept iterators = (... && iterator<Vs>);
template <typename... Is>
concept multipass_iterators = iterators<Is...> && (... && multipass_iterator<Is>);
template <typename... Is>
concept reversible_iterators = multipass_iterators<Is...> && (... && reversible_iterator<Is>);
template <typename... Is>
concept random_access_iterators =
    reversible_iterators<Is...> && (... && random_access_iterator<Is>);

template <view V1, view... Vs>
    requires all_same<view_element_type_t<V1>, view_element_type_t<Vs>...>
struct concat_view_element_type {
    using type = view_element_type_t<V1>;
};
template <view V1, view... Vs>
    requires all_same<view_element_type_t<V1>, view_element_type_t<Vs>...>
using concat_view_element_type_t = typename concat_view_element_type<V1, Vs...>::type;

template <iterator I1, iterator... Is>
    requires all_same<iterator_element_type_t<I1>, iterator_element_type_t<Is>...>
struct concat_iterator_element_type {
    using type = iterator_element_type_t<I1>;
};
template <iterator... Is>
    requires all_same<iterator_element_type_t<Is>...>
using concat_iterator_element_type_t = typename concat_iterator_element_type<Is...>::type;

template <typename I1, typename... Is>
    requires iterators<I1, Is...>
struct concat_iterator_index_type {
    using type = no_index_type_t;
};
template <typename I1, typename... Is>
    requires random_access_iterators<I1, Is...>
struct concat_iterator_index_type<I1, Is...> {
    using type = std::common_type_t<iterator_index_type_t<I1>, iterator_index_type_t<Is>...>;
};
template <iterator... Is>
using concat_iterator_index_type_t = typename concat_iterator_index_type<Is...>::type;

struct concat_dummy {
    // TODO remove this constructor so we can check that we don't accidentally construct temporaries
    // template <typename... T>
    // constexpr concat_dummy(T&&...) noexcept {}
};

template <typename CurrentIt, typename FrontIt, typename BackIt>
struct concat_internal_iterator_triple {
    using Current = CurrentIt;
    using Front = FrontIt;
    using Back = BackIt;
    [[no_unique_address]] Current current;
    [[no_unique_address]] Front
        front;  // If this concat_internal_iterator_triple is in concat_forward_iterator, then only
                // available when current is a reversible iterator; if this
                // concat_internal_iterator_triple is in concat_backward_iterator, then only
                // available when current is an iterator.  Not available for first index.
    [[no_unique_address]] Back
        back;  // If this concat_internal_iterator_triple is in concat_forward_iterator, then only
    // available when current is an iterator; if this
    // concat_internal_iterator_triple is in concat_backward_iterator, then only
    // available when current is a reversible iterator.  Not available for last index.
    template <typename CurrentIt2, typename FrontIt2, typename BackIt2>
    constexpr concat_internal_iterator_triple(CurrentIt2&& c, FrontIt2&& f, BackIt2&& b) noexcept(
        std::is_nothrow_constructible_v<Current, CurrentIt2> &&
        std::is_nothrow_constructible_v<Front, FrontIt2> &&
        std::is_nothrow_constructible_v<Back, BackIt2>)
        : current(std::forward<CurrentIt2>(c)),
          front(std::forward<FrontIt2>(f)),
          back(std::forward<BackIt2>(b)) {}
};

template <size_t Index, typename... CurrentIts, typename ViewTuple, typename F>
    requires(sizeof...(CurrentIts) > 1)
constexpr decltype(auto) generate_forward_iterator_variant(ViewTuple& vs, F&& f) {
    if constexpr (Index == 0) {
        if constexpr (iterators<CurrentIts...>) {
            return std::forward<F>(f)(std::get<Index>(vs).forward_iter(),
                                      impl::concat_dummy{},
                                      std::get<Index>(vs).backward_iter());
        } else {
            return std::forward<F>(f)(
                std::get<Index>(vs).forward_iter(), impl::concat_dummy{}, impl::concat_dummy{});
        }
    } else if constexpr (Index + 1 == sizeof...(CurrentIts)) {
        if constexpr (reversible_iterators<CurrentIts...>) {
            auto curr = std::get<Index>(vs).forward_iter();
            auto front = curr;
            return std::forward<F>(f)(std::move(curr), std::move(front), impl::concat_dummy{});
        } else {
            return std::forward<F>(f)(
                std::get<Index>(vs).forward_iter(), impl::concat_dummy{}, impl::concat_dummy{});
        }
    } else {
        if constexpr (reversible_iterators<CurrentIts...>) {
            auto curr = std::get<Index>(vs).forward_iter();
            auto front = curr;
            return std::forward<F>(f)(
                std::move(curr), std::move(front), std::get<Index>(vs).backward_iter());
        } else if constexpr (iterators<CurrentIts...>) {
            return std::forward<F>(f)(std::get<Index>(vs).forward_iter(),
                                      impl::concat_dummy{},
                                      std::get<Index>(vs).backward_iter());
        } else {
            return std::forward<F>(f)(
                std::get<Index>(vs).forward_iter(), impl::concat_dummy{}, impl::concat_dummy{});
        }
    }
}

template <size_t I, typename ViewTuple, typename F>
constexpr decltype(auto) generate_forward_iterator_variant_from_tuple(ViewTuple& vs, F&& f) {
    return std::apply(
        [&](auto&... views) -> decltype(auto) {
            return generate_forward_iterator_variant<I, decltype(views.forward_iter())...>(
                vs, std::forward<F>(f));
        },
        vs);
}

template <size_t Index, typename... CurrentIts, typename ViewTuple, typename F>
    requires(sizeof...(CurrentIts) > 1)
constexpr decltype(auto) generate_backward_iterator_variant(ViewTuple& vs, F&& f) {
    if constexpr (Index == 0) {
        if constexpr (reversible_iterators<CurrentIts...>) {
            auto curr = std::get<Index>(vs).backward_iter();
            auto back = curr;
            return std::forward<F>(f)(std::move(curr), impl::concat_dummy{}, std::move(back));
        } else {
            return std::forward<F>(f)(
                std::get<Index>(vs).backward_iter(), impl::concat_dummy{}, impl::concat_dummy{});
        }
    } else if constexpr (Index + 1 == sizeof...(CurrentIts)) {
        if constexpr (iterators<CurrentIts...>) {
            return std::forward<F>(f)(std::get<Index>(vs).backward_iter(),
                                      std::get<Index>(vs).forward_iter(),
                                      impl::concat_dummy{});
        } else {
            return std::forward<F>(f)(
                std::get<Index>(vs).backward_iter(), impl::concat_dummy{}, impl::concat_dummy{});
        }
    } else {
        if constexpr (reversible_iterators<CurrentIts...>) {
            auto curr = std::get<Index>(vs).backward_iter();
            auto back = curr;
            return std::forward<F>(f)(
                std::move(curr), std::get<Index>(vs).forward_iter(), std::move(back));
        } else if constexpr (iterators<CurrentIts...>) {
            return std::forward<F>(f)(std::get<Index>(vs).backward_iter(),
                                      std::get<Index>(vs).forward_iter(),
                                      impl::concat_dummy{});
        } else {
            return std::forward<F>(f)(
                std::get<Index>(vs).backward_iter(), impl::concat_dummy{}, impl::concat_dummy{});
        }
    }
}

template <size_t I, typename ViewTuple, typename F>
constexpr decltype(auto) generate_backward_iterator_variant_from_tuple(ViewTuple& vs, F&& f) {
    return std::apply(
        [&](auto&... views) -> decltype(auto) {
            return generate_backward_iterator_variant<I, decltype(views.backward_iter())...>(
                vs, std::forward<F>(f));
        },
        vs);
}

template <bool Reversible, typename Triple>
struct concat_invert_triple;
template <typename CurrentIt, typename FrontIt, typename BackIt>
struct concat_invert_triple<false, concat_internal_iterator_triple<CurrentIt, FrontIt, BackIt>> {
    using type = concat_internal_iterator_triple<decltype(std::declval<CurrentIt>().invert()),
                                                 concat_dummy,
                                                 concat_dummy>;
};
template <typename CurrentIt, typename FrontIt, typename BackIt>
struct concat_invert_triple<true, concat_internal_iterator_triple<CurrentIt, FrontIt, BackIt>> {
    using type = concat_internal_iterator_triple<decltype(std::declval<CurrentIt>().invert()),
                                                 FrontIt,
                                                 BackIt>;
};
template <bool Reversible, typename Triple>
using concat_invert_triple_t = typename concat_invert_triple<Reversible, Triple>::type;

template <typename, typename...>
class concat_forward_iterator;
template <typename, typename...>
class concat_backward_iterator;

template <typename ViewTuple, typename... Triples>
class concat_forward_iterator {
   private:
    template <typename... Vs>
        requires impl::concatable_views<Vs...>
    friend class duality::concat_view;
    template <typename, typename...>
    friend class concat_backward_iterator;
    [[no_unique_address]] std::variant<Triples...> is_;

    template <size_t Index, typename CurrentIt, typename FrontIt, typename BackIt>
    constexpr concat_forward_iterator(
        ViewTuple&,
        std::in_place_index_t<Index>,
        CurrentIt&& current,
        FrontIt&& front,
        BackIt&&
            back) noexcept(std::
                               is_nothrow_constructible_v<
                                   std::tuple_element_t<Index,
                                                        std::tuple<typename Triples::Current...>>,
                                   CurrentIt> &&
                           std::is_nothrow_constructible_v<
                               std::tuple_element_t<Index, std::tuple<typename Triples::Front...>>,
                               FrontIt> &&
                           std::is_nothrow_constructible_v<
                               std::tuple_element_t<Index, std::tuple<typename Triples::Back...>>,
                               BackIt>)
        : is_(std::in_place_index<Index>,
              std::forward<CurrentIt>(current),
              std::forward<FrontIt>(front),
              std::forward<BackIt>(back)) {}
};

// Note: std::get<Index> for variant can be replaced with an unchecked version
template <typename ViewTuple, typename... Triples>
    requires iterators<typename Triples::Current...>
class concat_forward_iterator<ViewTuple, Triples...> {
   private:
    template <typename... Vs>
        requires impl::concatable_views<Vs...>
    friend class duality::concat_view;
    template <typename, typename...>
    friend class concat_backward_iterator;
    using element_type = concat_iterator_element_type_t<typename Triples::Current...>;
    ViewTuple* vs_;
    [[no_unique_address]] std::variant<Triples...> is_;
    constexpr static size_t num_views = sizeof...(Triples);

    template <size_t Index, typename CurrentIt, typename FrontIt, typename BackIt>
    constexpr concat_forward_iterator(
        ViewTuple& vs,
        std::in_place_index_t<Index>,
        CurrentIt&& current,
        FrontIt&& front,
        BackIt&&
            back) noexcept(std::
                               is_nothrow_constructible_v<
                                   std::tuple_element_t<Index,
                                                        std::tuple<typename Triples::Current...>>,
                                   CurrentIt> &&
                           std::is_nothrow_constructible_v<
                               std::tuple_element_t<Index, std::tuple<typename Triples::Front...>>,
                               FrontIt> &&
                           std::is_nothrow_constructible_v<
                               std::tuple_element_t<Index, std::tuple<typename Triples::Back...>>,
                               BackIt>)
        : vs_(&vs),
          is_(std::in_place_index<Index>,
              std::forward<CurrentIt>(current),
              std::forward<FrontIt>(front),
              std::forward<BackIt>(back)) {}

    template <size_t Index>
    constexpr decltype(auto) emplace_variant() {
        return generate_forward_iterator_variant<Index, typename Triples::Current...>(
            *vs_, [this](auto&& current, auto&& front, auto&& back) -> decltype(auto) {
                return is_.template emplace<Index>(std::forward<decltype(current)>(current),
                                                   std::forward<decltype(front)>(front),
                                                   std::forward<decltype(back)>(back));
            });
    }

   public:
    using index_type = concat_iterator_index_type_t<typename Triples::Current...>;

   private:
    template <size_t Index, typename Triple>
    constexpr element_type next_impl(Triple& triple) {
        if constexpr (Index + 1 == num_views) {
            return triple.current.next();
        } else {
            if (auto opt = triple.current.next(triple.back)) {
                return *std::move(opt);
            } else {
                constexpr size_t NewIndex = Index + 1;
                return next_impl<NewIndex>(emplace_variant<NewIndex>());
            }
        }
    }

   public:
    constexpr element_type next() {
        return visit_with_index(
            []<size_t Index>(auto& triple, concat_forward_iterator* that) -> decltype(auto) {
                return that->next_impl<Index>(triple);
            },
            is_,
            this);
    }

   private:
    template <size_t Index, typename Triple, typename... Triples2>
    constexpr optional<element_type> next_impl(
        Triple& triple,
        const concat_backward_iterator<ViewTuple, Triples2...>& end_i) {
        if constexpr (Index + 1 == num_views) {
            // If we are at the last underlying view, then a valid sentinel must also be at the last
            // underlying view.
            return triple.current.next(std::get<Index>(end_i.is_).current);
        } else if (Index == end_i.is_.index()) {
            // We are at the same underlying view as the sentinel.
            return triple.current.next(std::get<Index>(end_i.is_).current);
        } else {
            // We are not on the same underlying view (it is then assumed that we are in an
            // earlier underlying view than the sentinel).

            if (auto opt = triple.current.next(triple.back)) {
                return opt;
            } else {
                constexpr size_t NewIndex = Index + 1;
                return next_impl<NewIndex>(emplace_variant<NewIndex>(), end_i);
            }
        }
    }

   public:
    template <typename... Triples2>
        requires((... && sentinel_for<typename Triples2::Current, typename Triples::Current>))
    constexpr optional<element_type> next(
        const concat_backward_iterator<ViewTuple, Triples2...>& end_i) {
        return visit_with_index(
            []<size_t Index>(auto& triple,
                             concat_forward_iterator* that,
                             const concat_backward_iterator<ViewTuple, Triples2...>& e)
                -> decltype(auto) { return that->next_impl<Index>(triple, e); },
            is_,
            this,
            end_i);
    }

   private:
    template <size_t Index, typename Triple>
    constexpr void skip_impl(Triple& triple) {
        if constexpr (Index + 1 == num_views) {
            triple.current.skip();
        } else {
            if (!triple.current.skip(triple.back)) {
                constexpr size_t NewIndex = Index + 1;
                return skip_impl<NewIndex>(emplace_variant<NewIndex>());
            }
        }
    }

   public:
    constexpr void skip() {
        visit_with_index(
            []<size_t Index>(auto& triple, concat_forward_iterator* that) -> decltype(auto) {
                that->skip_impl<Index>(triple);
            },
            is_,
            this);
    }

   private:
    template <size_t Index, typename Triple, typename... Triples2>
    constexpr bool skip_impl(Triple& triple,
                             const concat_backward_iterator<ViewTuple, Triples2...>& end_i) {
        if constexpr (Index + 1 == num_views) {
            // If we are at the last underlying view, then a valid sentinel must also be at the last
            // underlying view.
            return triple.current.skip(std::get<Index>(end_i.is_).current);
        } else if (Index == end_i.is_.index()) {
            // We are at the same underlying view as the sentinel.
            return triple.current.skip(std::get<Index>(end_i.is_).current);
        } else {
            // We are not on the same underlying view (it is then assumed that we are in an
            // earlier underlying view than the sentinel).

            if (triple.current.next(triple.back)) {
                return true;
            } else {
                constexpr size_t NewIndex = Index + 1;
                return skip_impl<NewIndex>(emplace_variant<NewIndex>(), end_i);
            }
        }
    }

   public:
    template <typename... Triples2>
        requires((... && sentinel_for<typename Triples2::Current, typename Triples::Current>))
    constexpr bool skip(const concat_backward_iterator<ViewTuple, Triples2...>& end_i) {
        return visit_with_index(
            []<size_t Index>(auto& triple,
                             concat_forward_iterator* that,
                             const concat_backward_iterator<ViewTuple, Triples2...>& e)
                -> decltype(auto) { return that->skip_impl<Index>(triple, e); },
            is_,
            this,
            end_i);
    }

    // TODO everything below this line, except invert().
   private:
    template <size_t Index, typename Triple>
    constexpr void skip_impl(Triple& triple, index_type amount) {
        if constexpr (Index + 1 == num_views) {
            triple.current.skip(amount);
        } else {
            if (auto actual_amount = triple.current.skip(amount, triple.back);
                actual_amount != amount) {
                constexpr size_t NewIndex = Index + 1;
                skip_impl<NewIndex>(emplace_variant<NewIndex>(), amount - actual_amount);
            }
        }
    }

   public:
    constexpr void skip(index_type amount)
        requires((... && random_access_iterator<typename Triples::Current>))
    {
        visit_with_index([]<size_t Index>(auto& triple, concat_forward_iterator* that, index_type a)
                             -> decltype(auto) { that->skip_impl<Index>(triple, a); },
                         is_,
                         this,
                         amount);
    }

   private:
    template <size_t Index, typename Triple, typename... Triples2>
    constexpr index_type skip_impl(Triple& triple,
                                   index_type amount,
                                   const concat_backward_iterator<ViewTuple, Triples2...>& end_i) {
        if constexpr (Index + 1 == num_views) {
            // If we are at the last underlying view, then a valid sentinel must also be at the last
            // underlying view.
            return triple.current.skip(amount, std::get<Index>(end_i.is_).current);
        } else if (Index == end_i.is_.index()) {
            // We are at the same underlying view as the sentinel.
            return triple.current.skip(amount, std::get<Index>(end_i.is_).current);
        } else {
            // We are not on the same underlying view (it is then assumed that we are in an
            // earlier underlying view than the sentinel).

            if (auto actual_amount = triple.current.skip(amount, triple.back);
                actual_amount != amount) {
                constexpr size_t NewIndex = Index + 1;
                return actual_amount + skip_impl<NewIndex>(emplace_variant<NewIndex>(),
                                                           amount - actual_amount,
                                                           end_i);
            } else {
                return amount;
            }
        }
    }

   public:
    template <typename... Triples2>
        requires((... && sentinel_for<typename Triples2::Current, typename Triples::Current>))
    constexpr index_type skip(index_type amount,
                              const concat_backward_iterator<ViewTuple, Triples2...>& end_i)
        requires((... && random_access_iterator<typename Triples::Current>))
    {
        return visit_with_index(
            []<size_t Index>(auto& triple,
                             concat_forward_iterator* that,
                             index_type a,
                             const concat_backward_iterator<ViewTuple, Triples2...>& e)
                -> decltype(auto) { return that->skip_impl<Index>(triple, a, e); },
            is_,
            this,
            amount,
            end_i);
    }
    constexpr decltype(auto) invert() const
        requires((... && multipass_iterator<typename Triples::Current>))
    {
        return visit_with_index(
            []<size_t Index>(auto& triple, const concat_forward_iterator* that) -> decltype(auto) {
                return concat_backward_iterator<
                    ViewTuple,
                    concat_invert_triple_t<reversible_iterators<typename Triples::Current>,
                                           Triples>...>(*that->vs_,
                                                        std::in_place_index<Index>,
                                                        triple.current.invert(),
                                                        triple.front,
                                                        triple.back);
            },
            is_,
            this);
    }
};

template <typename ViewTuple, typename... Triples>
class concat_backward_iterator {
   private:
    template <typename... Vs>
        requires impl::concatable_views<Vs...>
    friend class duality::concat_view;
    template <typename, typename...>
    friend class concat_forward_iterator;
    [[no_unique_address]] std::variant<Triples...> is_;

    template <size_t Index, typename CurrentIt, typename FrontIt, typename BackIt>
    constexpr concat_backward_iterator(
        ViewTuple&,
        std::in_place_index_t<Index>,
        CurrentIt&& current,
        FrontIt&& front,
        BackIt&&
            back) noexcept(std::
                               is_nothrow_constructible_v<
                                   std::tuple_element_t<Index,
                                                        std::tuple<typename Triples::Current...>>,
                                   CurrentIt> &&
                           std::is_nothrow_constructible_v<
                               std::tuple_element_t<Index, std::tuple<typename Triples::Front...>>,
                               FrontIt> &&
                           std::is_nothrow_constructible_v<
                               std::tuple_element_t<Index, std::tuple<typename Triples::Back...>>,
                               BackIt>)
        : is_(std::in_place_index<Index>,
              std::forward<CurrentIt>(current),
              std::forward<FrontIt>(front),
              std::forward<BackIt>(back)) {}
};

// Note: std::get<Index> for variant can be replaced with an unchecked version
template <typename ViewTuple, typename... Triples>
    requires iterators<typename Triples::Current...>
class concat_backward_iterator<ViewTuple, Triples...> {
   private:
    template <typename... Vs>
        requires impl::concatable_views<Vs...>
    friend class duality::concat_view;
    template <typename, typename...>
    friend class concat_forward_iterator;
    using element_type = concat_iterator_element_type_t<typename Triples::Current...>;
    ViewTuple* vs_;
    [[no_unique_address]] std::variant<Triples...> is_;
    constexpr static size_t num_views = sizeof...(Triples);

    template <size_t Index, typename CurrentIt, typename FrontIt, typename BackIt>
    constexpr concat_backward_iterator(
        ViewTuple& vs,
        std::in_place_index_t<Index>,
        CurrentIt&& current,
        FrontIt&& front,
        BackIt&&
            back) noexcept(std::
                               is_nothrow_constructible_v<
                                   std::tuple_element_t<Index,
                                                        std::tuple<typename Triples::Current...>>,
                                   CurrentIt> &&
                           std::is_nothrow_constructible_v<
                               std::tuple_element_t<Index, std::tuple<typename Triples::Front...>>,
                               FrontIt> &&
                           std::is_nothrow_constructible_v<
                               std::tuple_element_t<Index, std::tuple<typename Triples::Back...>>,
                               BackIt>)
        : vs_(&vs),
          is_(std::in_place_index<Index>,
              std::forward<CurrentIt>(current),
              std::forward<FrontIt>(front),
              std::forward<BackIt>(back)) {}

    template <size_t Index>
    constexpr decltype(auto) emplace_variant() {
        return generate_backward_iterator_variant<Index, typename Triples::Current...>(
            *vs_, [this](auto&& current, auto&& front, auto&& back) -> decltype(auto) {
                return is_.template emplace<Index>(std::forward<decltype(current)>(current),
                                                   std::forward<decltype(front)>(front),
                                                   std::forward<decltype(back)>(back));
            });
    }

   public:
    using index_type = concat_iterator_index_type_t<typename Triples::Current...>;

   private:
    template <size_t Index, typename Triple>
    constexpr element_type next_impl(Triple& triple) {
        if constexpr (Index == 0) {
            return triple.current.next();
        } else {
            if (auto opt = triple.current.next(triple.front)) {
                return *std::move(opt);
            } else {
                constexpr size_t NewIndex = Index - 1;
                return next_impl<NewIndex>(emplace_variant<NewIndex>());
            }
        }
    }

   public:
    constexpr element_type next() {
        return visit_with_index(
            []<size_t Index>(auto& triple, concat_backward_iterator* that) -> decltype(auto) {
                return that->next_impl<Index>(triple);
            },
            is_,
            this);
    }

   private:
    template <size_t Index, typename Triple, typename... Triples2>
    constexpr optional<element_type> next_impl(
        Triple& triple,
        const concat_forward_iterator<ViewTuple, Triples2...>& end_i) {
        if constexpr (Index == 0) {
            // If we are at the last underlying view, then a valid sentinel must also be at the last
            // underlying view.
            return triple.current.next(std::get<Index>(end_i.is_).current);
        } else if (Index == end_i.is_.index()) {
            // We are at the same underlying view as the sentinel.
            return triple.current.next(std::get<Index>(end_i.is_).current);
        } else {
            // We are not on the same underlying view (it is then assumed that we are in an
            // earlier underlying view than the sentinel).

            if (auto opt = triple.current.next(triple.front)) {
                return opt;
            } else {
                constexpr size_t NewIndex = Index - 1;
                return next_impl<NewIndex>(emplace_variant<NewIndex>(), end_i);
            }
        }
    }

   public:
    template <typename... Triples2>
        requires((... && sentinel_for<typename Triples2::Current, typename Triples::Current>))
    constexpr optional<element_type> next(
        const concat_forward_iterator<ViewTuple, Triples2...>& end_i) {
        return visit_with_index(
            []<size_t Index>(auto& triple,
                             concat_backward_iterator* that,
                             const concat_forward_iterator<ViewTuple, Triples2...>& e)
                -> decltype(auto) { return that->next_impl<Index>(triple, e); },
            is_,
            this,
            end_i);
    }

   private:
    template <size_t Index, typename Triple>
    constexpr void skip_impl(Triple& triple) {
        if constexpr (Index == 0) {
            triple.current.skip();
        } else {
            if (!triple.current.skip(triple.front)) {
                constexpr size_t NewIndex = Index - 1;
                return skip_impl<NewIndex>(emplace_variant<NewIndex>());
            }
        }
    }

   public:
    constexpr void skip() {
        visit_with_index(
            []<size_t Index>(auto& triple, concat_backward_iterator* that) -> decltype(auto) {
                that->skip_impl<Index>(triple);
            },
            is_,
            this);
    }

   private:
    template <size_t Index, typename Triple, typename... Triples2>
    constexpr bool skip_impl(Triple& triple,
                             const concat_forward_iterator<ViewTuple, Triples2...>& end_i) {
        if constexpr (Index == 0) {
            // If we are at the last underlying view, then a valid sentinel must also be at the last
            // underlying view.
            return triple.current.skip(std::get<Index>(end_i.is_).current);
        } else if (Index == end_i.is_.index()) {
            // We are at the same underlying view as the sentinel.
            return triple.current.skip(std::get<Index>(end_i.is_).current);
        } else {
            // We are not on the same underlying view (it is then assumed that we are in an
            // earlier underlying view than the sentinel).

            if (triple.current.next(triple.front)) {
                return true;
            } else {
                constexpr size_t NewIndex = Index - 1;
                return skip_impl<NewIndex>(emplace_variant<NewIndex>(), end_i);
            }
        }
    }

   public:
    template <typename... Triples2>
        requires((... && sentinel_for<typename Triples2::Current, typename Triples::Current>))
    constexpr bool skip(const concat_forward_iterator<ViewTuple, Triples2...>& end_i) {
        return visit_with_index(
            []<size_t Index>(auto& triple,
                             concat_backward_iterator* that,
                             const concat_forward_iterator<ViewTuple, Triples2...>& e)
                -> decltype(auto) { return that->skip_impl<Index>(triple, e); },
            is_,
            this,
            end_i);
    }

   private:
    template <size_t Index, typename Triple>
    constexpr void skip_impl(Triple& triple, index_type amount) {
        if constexpr (Index == 0) {
            triple.current.skip(amount);
        } else {
            if (auto actual_amount = triple.current.skip(amount, triple.front);
                actual_amount != amount) {
                constexpr size_t NewIndex = Index - 1;
                skip_impl<NewIndex>(emplace_variant<NewIndex>(), amount - actual_amount);
            }
        }
    }

   public:
    constexpr void skip(index_type amount)
        requires((... && random_access_iterator<typename Triples::Current>))
    {
        visit_with_index(
            []<size_t Index>(auto& triple,
                             concat_backward_iterator* that,
                             index_type a) -> decltype(auto) { that->skip_impl<Index>(triple, a); },
            is_,
            this,
            amount);
    }

   private:
    template <size_t Index, typename Triple, typename... Triples2>
    constexpr index_type skip_impl(Triple& triple,
                                   index_type amount,
                                   const concat_forward_iterator<ViewTuple, Triples2...>& end_i) {
        if constexpr (Index == 0) {
            // If we are at the last underlying view, then a valid sentinel must also be at the last
            // underlying view.
            return triple.current.skip(amount, std::get<Index>(end_i.is_).current);
        } else if (Index == end_i.is_.index()) {
            // We are at the same underlying view as the sentinel.
            return triple.current.skip(amount, std::get<Index>(end_i.is_).current);
        } else {
            // We are not on the same underlying view (it is then assumed that we are in an
            // earlier underlying view than the sentinel).

            if (auto actual_amount = triple.current.skip(amount, triple.front);
                actual_amount != amount) {
                constexpr size_t NewIndex = Index - 1;
                return actual_amount + skip_impl<NewIndex>(emplace_variant<NewIndex>(),
                                                           amount - actual_amount,
                                                           end_i);
            } else {
                return amount;
            }
        }
    }

   public:
    template <typename... Triples2>
        requires((... && sentinel_for<typename Triples2::Current, typename Triples::Current>))
    constexpr index_type skip(index_type amount,
                              const concat_forward_iterator<ViewTuple, Triples2...>& end_i)
        requires((... && random_access_iterator<typename Triples::Current>))
    {
        return visit_with_index(
            []<size_t Index>(auto& triple,
                             concat_backward_iterator* that,
                             index_type a,
                             const concat_forward_iterator<ViewTuple, Triples2...>& e)
                -> decltype(auto) { return that->skip_impl<Index>(triple, a, e); },
            is_,
            this,
            amount,
            end_i);
    }
    constexpr decltype(auto) invert() const
        requires((... && multipass_iterator<typename Triples::Current>))
    {
        return visit_with_index(
            []<size_t Index>(auto& triple, const concat_backward_iterator* that) -> decltype(auto) {
                return concat_forward_iterator<
                    ViewTuple,
                    concat_invert_triple_t<reversible_iterators<typename Triples::Current>,
                                           Triples>...>(*that->vs_,
                                                        std::in_place_index<Index>,
                                                        triple.current.invert(),
                                                        triple.front,
                                                        triple.back);
            },
            is_,
            this);
    }
};

/// @brief Struct whose dependent type is the specialisation of concat_forward_iterator to use, but
/// with concat_dummy types put in the triple as necessary.
/// @tparam ViewTuple A std::tuple of views, possibly cvref-qualified.
/// @tparam N The number of elements in ViewTuple.
/// @tparam IndexSeq A std::index_sequence from 0 to N-1.
template <typename ViewTuple, size_t N, typename IndexSeq>
struct make_concat_forward_iterator;
template <typename ViewTuple, size_t N, size_t... Is>
struct make_concat_forward_iterator<ViewTuple, N, std::index_sequence<Is...>> {
    constexpr static bool is_iterator =
        (... && iterator<decltype(std::get<Is>(std::declval<ViewTuple>()).forward_iter())>);
    constexpr static bool is_reversible_iterator =
        (... &&
         reversible_iterator<decltype(std::get<Is>(std::declval<ViewTuple>()).forward_iter())>);
    using type = concat_forward_iterator<
        ViewTuple,
        concat_internal_iterator_triple<
            decltype(std::get<Is>(std::declval<ViewTuple>()).forward_iter()),
            std::conditional_t<is_reversible_iterator && Is != 0,
                               decltype(std::get<Is>(std::declval<ViewTuple>()).forward_iter()),
                               concat_dummy>,
            std::conditional_t<is_iterator && Is + 1 != N,
                               decltype(std::get<Is>(std::declval<ViewTuple>()).backward_iter()),
                               concat_dummy>>...>;
};
template <typename ViewTuple>
using make_concat_forward_iterator_t = typename make_concat_forward_iterator<
    ViewTuple,
    std::tuple_size_v<std::remove_cvref_t<ViewTuple>>,
    std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<ViewTuple>>>>::type;
template <typename ViewTuple>
constexpr bool is_concat_forward_iterator_v = make_concat_forward_iterator<
    ViewTuple,
    std::tuple_size_v<std::remove_cvref_t<ViewTuple>>,
    std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<ViewTuple>>>>::is_iterator;
template <typename ViewTuple>
constexpr bool is_concat_forward_reversible_iterator_v =
    make_concat_forward_iterator<ViewTuple,
                                 std::tuple_size_v<std::remove_cvref_t<ViewTuple>>,
                                 std::make_index_sequence<std::tuple_size_v<
                                     std::remove_cvref_t<ViewTuple>>>>::is_reversible_iterator;

/// @brief Struct whose dependent type is the specialisation of concat_backward_iterator to use, but
/// with concat_dummy types put in the triple as necessary.
/// @tparam ViewTuple A std::tuple of views, possibly cvref-qualified.
/// @tparam N The number of elements in ViewTuple.
/// @tparam IndexSeq A std::index_sequence from 0 to N-1.
template <typename ViewTuple, size_t N, typename IndexSeq>
struct make_concat_backward_iterator;
template <typename ViewTuple, size_t N, size_t... Is>
struct make_concat_backward_iterator<ViewTuple, N, std::index_sequence<Is...>> {
    constexpr static bool is_iterator =
        (... && iterator<decltype(std::get<Is>(std::declval<ViewTuple>()).backward_iter())>);
    constexpr static bool is_reversible_iterator =
        (... &&
         reversible_iterator<decltype(std::get<Is>(std::declval<ViewTuple>()).backward_iter())>);
    using type = concat_backward_iterator<
        ViewTuple,
        concat_internal_iterator_triple<
            decltype(std::get<Is>(std::declval<ViewTuple>()).backward_iter()),
            std::conditional_t<is_iterator && Is != 0,
                               decltype(std::get<Is>(std::declval<ViewTuple>()).forward_iter()),
                               concat_dummy>,
            std::conditional_t<is_reversible_iterator && Is + 1 != N,
                               decltype(std::get<Is>(std::declval<ViewTuple>()).backward_iter()),
                               concat_dummy>>...>;
};
template <typename ViewTuple>
using make_concat_backward_iterator_t = typename make_concat_backward_iterator<
    ViewTuple,
    std::tuple_size_v<std::remove_cvref_t<ViewTuple>>,
    std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<ViewTuple>>>>::type;
template <typename ViewTuple>
constexpr bool is_concat_backward_iterator_v = make_concat_backward_iterator<
    ViewTuple,
    std::tuple_size_v<std::remove_cvref_t<ViewTuple>>,
    std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<ViewTuple>>>>::is_iterator;
template <typename ViewTuple>
constexpr bool is_concat_backward_reversible_iterator_v =
    make_concat_backward_iterator<ViewTuple,
                                  std::tuple_size_v<std::remove_cvref_t<ViewTuple>>,
                                  std::make_index_sequence<std::tuple_size_v<
                                      std::remove_cvref_t<ViewTuple>>>>::is_reversible_iterator;

}  // namespace impl

/// @brief  concat_view with two or more subviews.
/// @tparam V
template <typename... Vs>
    requires impl::multi_concatable_views<Vs...>
class concat_view<Vs...> {
   private:
    // Note: std::tuple probably doesn't compress empty elements.
    [[no_unique_address]] std::tuple<Vs...> vs_;

   public:
    template <view... Vs2>
    constexpr concat_view(wrapping_construct_t,
                          Vs2&&... vs) noexcept((... && std::is_nothrow_constructible_v<Vs2>))
        : vs_(std::forward<Vs2>(vs)...) {}
    constexpr decltype(auto) forward_iter() {
        using forward_iterator_t =
            impl::make_concat_forward_iterator_t<std::remove_reference_t<decltype((vs_))>>;
        return impl::generate_forward_iterator_variant_from_tuple<0, decltype((vs_))>(
            vs_, [this](auto&& current, auto&& front, auto&& back) {
                return forward_iterator_t(vs_,
                                          std::in_place_index<0>,
                                          std::forward<decltype(current)>(current),
                                          std::forward<decltype(front)>(front),
                                          std::forward<decltype(back)>(back));
            });
    }
    constexpr decltype(auto) forward_iter() const {
        using forward_iterator_t =
            impl::make_concat_forward_iterator_t<std::remove_reference_t<decltype((vs_))>>;
        return impl::generate_forward_iterator_variant_from_tuple<0, decltype((vs_))>(
            vs_, [this](auto&& current, auto&& front, auto&& back) {
                return forward_iterator_t(vs_,
                                          std::in_place_index<0>,
                                          std::forward<decltype(current)>(current),
                                          std::forward<decltype(front)>(front),
                                          std::forward<decltype(back)>(back));
            });
    }
    constexpr decltype(auto) backward_iter() {
        using backward_iterator_t =
            impl::make_concat_backward_iterator_t<std::remove_reference_t<decltype((vs_))>>;
        return impl::generate_backward_iterator_variant_from_tuple<sizeof...(Vs) - 1,
                                                                   decltype((vs_))>(
            vs_, [this](auto&& current, auto&& front, auto&& back) {
                return backward_iterator_t(vs_,
                                           std::in_place_index<sizeof...(Vs) - 1>,
                                           std::forward<decltype(current)>(current),
                                           std::forward<decltype(front)>(front),
                                           std::forward<decltype(back)>(back));
            });
    }
    constexpr decltype(auto) backward_iter() const {
        using backward_iterator_t =
            impl::make_concat_backward_iterator_t<std::remove_reference_t<decltype((vs_))>>;
        return impl::generate_backward_iterator_variant_from_tuple<sizeof...(Vs) - 1,
                                                                   decltype((vs_))>(
            vs_, [this](auto&& current, auto&& front, auto&& back) {
                return backward_iterator_t(vs_,
                                           std::in_place_index<sizeof...(Vs) - 1>,
                                           std::forward<decltype(current)>(current),
                                           std::forward<decltype(front)>(front),
                                           std::forward<decltype(back)>(back));
            });
    }
    constexpr decltype(auto) empty() const
        requires((... && emptyness_view<Vs>))
    {
        return std::apply([](const auto&... vs) -> decltype(auto) { return (... && vs.empty()); },
                          vs_);
    }
    constexpr decltype(auto) size() const
        requires((... && sized_view<Vs>))
    {
        using Ret = std::common_type_t<decltype(std::declval<Vs>().size())...>;
        return std::apply(
            [](const auto&... vs) -> decltype(auto) {
                return static_cast<Ret>((static_cast<Ret>(vs.size()) + ...));
            },
            vs_);
    }
    constexpr decltype(auto) size() const
        requires(infinite_view<Vs> || ...)
    {
        return infinite_t{};
    }
};

/// @brief  Trivial concat_view containing only one subview.
/// @tparam V
template <typename V>
    requires impl::single_concatable_views<V>
class concat_view<V> {
   private:
    [[no_unique_address]] V v_;

   public:
    template <view V2>
    constexpr concat_view(wrapping_construct_t,
                          V2&& v) noexcept(std::is_nothrow_constructible_v<V, V2>)
        : v_(std::forward<V2>(v)) {}
    constexpr decltype(auto) forward_iter() { return v_.forward_iter(); }
    constexpr decltype(auto) forward_iter() const { return v_.forward_iter(); }
    constexpr decltype(auto) backward_iter() { return v_.backward_iter(); }
    constexpr decltype(auto) backward_iter() const { return v_.backward_iter(); }
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

template <view... Vs>
concat_view(wrapping_construct_t, Vs&&... vs) -> concat_view<Vs...>;

namespace impl {

/// @brief This is the number of views up to and including the first infinite view.  This allows
/// views::concat to drop any views after the first infinite view immediately.  This allows
/// reversibility for the concat iterator to be implemented correctly, since for an infinite view,
/// reversing the iterator may not yield the same type as the end sentinel.
template <view...>
struct reduced_view_count;
template <>
struct reduced_view_count<> {
    constexpr static size_t value = 0;
};
template <infinite_view V, view... Vs>
struct reduced_view_count<V, Vs...> {
    constexpr static size_t value = 1;
};
template <view V, view... Vs>
    requires(!infinite_view<V>)
struct reduced_view_count<V, Vs...> {
    constexpr static size_t value = 1 + reduced_view_count<Vs...>::value;
};
template <view... Vs>
constexpr size_t reduced_view_count_v = reduced_view_count<Vs...>::value;

struct concat {
    template <view V, view... Vs>
    constexpr DUALITY_STATIC_CALL auto operator()(V&& v, Vs&&... vs) DUALITY_CONST_CALL {
        // This is a std::tuple containing a prefix of Vs..., up to and including the first infinite
        // view.  This is fine as it is never possible to access anything after the first infinite
        // view.
        using Tuple = std::tuple<V&&, Vs&&...>;
        Tuple viewrefs(std::forward<V>(v), std::forward<Vs>(vs)...);
        constexpr size_t num_reduced_views = reduced_view_count_v<V, Vs...>;
        return [&]<size_t... I>(std::index_sequence<I...>) {
            return concat_view(
                wrapping_construct,
                std::forward<std::tuple_element_t<I, Tuple>>(std::get<I>(viewrefs))...);
        }(std::make_index_sequence<num_reduced_views>{});
    }
};

}  // namespace impl

namespace views {
constexpr inline impl::concat concat;
}

}  // namespace duality
