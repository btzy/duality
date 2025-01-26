// This file is part of https://github.com/btzy/duality

#include <list>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/empty.hpp>
#include <duality/factories/repeat.hpp>
#include <duality/range.hpp>
#include <duality/viewifiers/forward_list.hpp>
#include <duality/viewifiers/vector.hpp>
#include <duality/views/eager_split_by.hpp>
#include <duality/views/reverse.hpp>
#include "../view_assert.hpp"

using namespace duality;

template <multipass_forward_view V1, multipass_forward_view V2, typename Equals = std::equal_to<>>
constexpr bool multipass_forward_view_val_equals(V1&& v1, V2&& v2, Equals&& equals = {}) noexcept {
    auto v1_fit = v1.forward_iter();
    auto v1_bit = v1.backward_iter();
    auto v2_fit = v2.forward_iter();
    auto v2_bit = v2.backward_iter();
    while (true) {
        optional v1_n = v1_fit.next(v1_bit);
        optional v2_n = v2_fit.next(v2_bit);
        bool v1_ended(v1_n);
        bool v2_ended(v2_n);
        if (v1_ended != v2_ended) return false;
        if (!v1_ended) return true;
        if (!equals(*v1_n, *v2_n)) return false;
    }
}

template <multipass_forward_view V1, multipass_forward_view V2>
constexpr bool multipass_forward_view_ref_equals(V1&& v1, V2&& v2) noexcept {
    auto v1_fit = v1.forward_iter();
    auto v1_bit = v1.backward_iter();
    auto v2_fit = v2.forward_iter();
    auto v2_bit = v2.backward_iter();
    while (true) {
        optional v1_n = v1_fit.next(v1_bit);
        optional v2_n = v2_fit.next(v2_bit);
        bool v1_ended(v1_n);
        bool v2_ended(v2_n);
        if (v1_ended != v2_ended) return false;
        if (!v1_ended) return true;
        if (&*v1_n != &*v2_n) return false;
    }
}

constexpr struct view_val_equals_t {
    template <view V1, view V2>
    constexpr bool operator()(V1&& v1, V2&& v2) const noexcept {
        static_assert((multipass_forward_view<V1> && multipass_forward_view<V2>) ||
                      (multipass_backward_view<V1> && multipass_backward_view<V2>));
        bool ok = true;
        if constexpr (multipass_forward_view<V1> && multipass_forward_view<V2>) {
            ok = multipass_forward_view_val_equals(v1, v2) && ok;
        }
        if constexpr (multipass_backward_view<V1> && multipass_backward_view<V2>) {
            ok = multipass_forward_view_val_equals(v1 | views::reverse, v2 | views::reverse) && ok;
        }
        return ok;
    }
} view_val_equals;

constexpr struct view_ref_equals_t {
    template <view V1, view V2>
    constexpr bool operator()(V1&& v1, V2&& v2) const noexcept {
        static_assert((multipass_forward_view<V1> && multipass_forward_view<V2>) ||
                      (multipass_backward_view<V1> && multipass_backward_view<V2>));
        bool ok = true;
        if constexpr (multipass_forward_view<V1> && multipass_forward_view<V2>) {
            ok = multipass_forward_view_ref_equals(v1, v2) && ok;
        }
        if constexpr (multipass_backward_view<V1> && multipass_backward_view<V2>) {
            ok = multipass_forward_view_ref_equals(v1 | views::reverse, v2 | views::reverse) && ok;
        }
        return ok;
    }
} view_ref_equals;

constexpr struct view_ref_equals_preserving_t {
    template <view V1, view V2>
    constexpr bool operator()(V1&& v1, V2&& v2) const noexcept {
        static_assert(std::same_as<decltype(std::declval<V1>().forward_iter()),
                                   decltype(std::declval<V2>().forward_iter())>);
        static_assert(std::same_as<decltype(std::declval<V1>().backward_iter()),
                                   decltype(std::declval<V2>().backward_iter())>);
        return view_ref_equals(std::forward<V1>(v1), std::forward<V2>(v2));
    }
} view_ref_equals_preserving;

template <random_access_forward_view V>
constexpr auto subrange(V&& v, size_t pos, size_t count) {
    auto fit = v.forward_iter();
    auto bit = v.backward_iter();
    REQUIRE(fit.skip(pos, bit) == pos);
    auto begin = fit;
    REQUIRE(fit.skip(count, bit) == count);
    return range{begin, fit.invert()};
}

template <multipass_forward_view V>
constexpr auto subrange(V&& v, size_t pos, size_t count) {
    auto fit = v.forward_iter();
    auto bit = v.backward_iter();
    for (size_t i = 0; i != pos; ++i) {
        REQUIRE(fit.skip(bit));
    }
    auto begin = fit;
    for (size_t i = 0; i != count; ++i) {
        REQUIRE(fit.skip(bit));
    }
    return range{begin, fit.invert()};
}

TEST_CASE("eager_split_by view of random_access_bidirectional_view", "[view eager_split_by]") {
    std::vector<int> vec{1, 2, 3, 5, 6, 8, 9, 10};
    auto v = viewify(vec);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);

    view_assert_multipass_bidirectional(views::eager_split_by(v, [](int x) { return x % 2 == 0; }),
                                        {subrange(v, 0, 1),
                                         subrange(v, 2, 2),
                                         subrange(v, 5, 0),
                                         subrange(v, 6, 1),
                                         subrange(v, 8, 0)},
                                        view_ref_equals_preserving);
    view_assert_multipass_bidirectional(views::eager_split_by(v, [](int x) { return x % 2 == 0; }),
                                        std::vector{viewify(std::vector<int>{1}),
                                                    viewify(std::vector<int>{3, 5}),
                                                    viewify(std::vector<int>{}),
                                                    viewify(std::vector<int>{9}),
                                                    viewify(std::vector<int>{})},
                                        view_val_equals);
    view_assert_multipass_bidirectional(views::eager_split_by(v, [](int x) { return x % 2 == 1; }),
                                        {subrange(v, 0, 0),
                                         subrange(v, 1, 1),
                                         subrange(v, 3, 0),
                                         subrange(v, 4, 2),
                                         subrange(v, 7, 1)},
                                        view_ref_equals_preserving);
    view_assert_multipass_bidirectional(views::eager_split_by(v, [](int x) { return x % 2 == 1; }),
                                        std::vector{viewify(std::vector<int>{}),
                                                    viewify(std::vector<int>{2}),
                                                    viewify(std::vector<int>{}),
                                                    viewify(std::vector<int>{6, 8}),
                                                    viewify(std::vector<int>{10})},
                                        view_val_equals);
    view_assert_multipass_bidirectional(v | views::eager_split_by([](int x) { return x % 2 == 1; }),
                                        std::vector{viewify(std::vector<int>{}),
                                                    viewify(std::vector<int>{2}),
                                                    viewify(std::vector<int>{}),
                                                    viewify(std::vector<int>{6, 8}),
                                                    viewify(std::vector<int>{10})},
                                        view_val_equals);
    view_assert_multipass_bidirectional(
        vec | views::eager_split_by([](int x) { return x % 2 == 1; }),
        std::vector{viewify(std::vector<int>{}),
                    viewify(std::vector<int>{2}),
                    viewify(std::vector<int>{}),
                    viewify(std::vector<int>{6, 8}),
                    viewify(std::vector<int>{10})},
        view_val_equals);
}

TEST_CASE("eager_split_by view of multipass_forward_view", "[view eager_split_by]") {
    std::forward_list<int> flist{1, 2, 3, 5, 6, 8, 9, 10};
    auto v = viewify(flist);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);

    view_assert_multipass_forward(views::eager_split_by(v, [](int x) { return x % 2 == 0; }),
                                  {subrange(v, 0, 1),
                                   subrange(v, 2, 2),
                                   subrange(v, 5, 0),
                                   subrange(v, 6, 1),
                                   subrange(v, 8, 0)},
                                  view_ref_equals_preserving);
    view_assert_multipass_forward(views::eager_split_by(v, [](int x) { return x % 2 == 0; }),
                                  std::vector{viewify(std::vector<int>{1}),
                                              viewify(std::vector<int>{3, 5}),
                                              viewify(std::vector<int>{}),
                                              viewify(std::vector<int>{9}),
                                              viewify(std::vector<int>{})},
                                  view_val_equals);
    view_assert_multipass_forward(views::eager_split_by(v, [](int x) { return x % 2 == 1; }),
                                  {subrange(v, 0, 0),
                                   subrange(v, 1, 1),
                                   subrange(v, 3, 0),
                                   subrange(v, 4, 2),
                                   subrange(v, 7, 1)},
                                  view_ref_equals_preserving);
    view_assert_multipass_forward(views::eager_split_by(v, [](int x) { return x % 2 == 1; }),
                                  std::vector{viewify(std::vector<int>{}),
                                              viewify(std::vector<int>{2}),
                                              viewify(std::vector<int>{}),
                                              viewify(std::vector<int>{6, 8}),
                                              viewify(std::vector<int>{10})},
                                  view_val_equals);
    view_assert_multipass_forward(v | views::eager_split_by([](int x) { return x % 2 == 1; }),
                                  std::vector{viewify(std::vector<int>{}),
                                              viewify(std::vector<int>{2}),
                                              viewify(std::vector<int>{}),
                                              viewify(std::vector<int>{6, 8}),
                                              viewify(std::vector<int>{10})},
                                  view_val_equals);
    view_assert_multipass_forward(flist | views::eager_split_by([](int x) { return x % 2 == 1; }),
                                  std::vector{viewify(std::vector<int>{}),
                                              viewify(std::vector<int>{2}),
                                              viewify(std::vector<int>{}),
                                              viewify(std::vector<int>{6, 8}),
                                              viewify(std::vector<int>{10})},
                                  view_val_equals);
}

TEST_CASE("eager_split_by view of infinite view is infinite", "[view eager_split_by]") {
    auto v = factories::repeat(5);
    auto res = v | views::eager_split_by([](int x) { return x % 2 == 0; });
    CHECK_FALSE(res.empty());
    auto sz = res.size();
    static_assert(std::same_as<decltype(sz), infinite_t>);
}

TEST_CASE("eager_split_by view of empty view is single empty range", "[view eager_split_by]") {
    auto v = factories::empty<int>();
    auto res = v | views::eager_split_by([](int x) { return x % 2 == 0; });
    CHECK_FALSE(res.empty());
    view_assert_multipass_bidirectional(
        res, std::vector{range{v.forward_iter(), v.backward_iter()}}, view_ref_equals_preserving);
}

TEST_CASE("eager_split_by view with function object by reference", "[view eager_split_by]") {
    auto v = factories::empty<int>();
    auto filter_fn = [](int x) { return x % 2 == 0; };
    auto res = v | views::eager_split_by(filter_fn);
    CHECK_FALSE(res.empty());
    view_assert_multipass_bidirectional(
        res, std::vector{range{v.forward_iter(), v.backward_iter()}}, view_ref_equals_preserving);
}
