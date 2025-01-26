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

struct view_ref_equals_t {
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

/*
TEST_CASE("filter view of random_access_bidirectional_view", "[view filter]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v = viewify(vec);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
    view_assert_multipass_bidirectional(views::filter(v, [](int x) { return x % 2 == 0; }), {2, 4});
    view_assert_multipass_bidirectional(views::filter(v, [](int x) { return x % 2 == 1; }),
                                        {1, 3, 5});
    view_assert_multipass_bidirectional(v | views::filter([](int x) { return x % 2 == 0; }),
                                        {2, 4});
    view_assert_multipass_bidirectional(vec | views::filter([](int x) { return x % 2 == 0; }),
                                        {2, 4});
    view_assert_multipass_bidirectional(std::vector<int>{10, 9, 8, 7, 6, 5, 4, 3, 2, 1} |
                                            views::filter([](int x) { return x % 2 == 0; }) |
                                            views::filter([](int x) { return x % 3 == 0; }),
                                        {6});
}

TEST_CASE("filter view of multipass_forward_view", "[view filter]") {
    std::forward_list<int> flist{1, 2, 3, 4, 5};
    auto v = viewify(flist);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
    view_assert_multipass_forward(views::filter(v, [](int x) { return x % 2 == 0; }), {2, 4});
    view_assert_multipass_forward(views::filter(v, [](int x) { return x % 2 == 1; }), {1, 3, 5});
    view_assert_multipass_forward(v | views::filter([](int x) { return x % 2 == 0; }), {2, 4});
    view_assert_multipass_forward(flist | views::filter([](int x) { return x % 2 == 0; }), {2, 4});
    view_assert_multipass_forward(std::forward_list<int>{10, 9, 8, 7, 6, 5, 4, 3, 2, 1} |
                                      views::filter([](int x) { return x % 2 == 0; }) |
                                      views::filter([](int x) { return x % 3 == 0; }),
                                  {6});
}

TEST_CASE("filter view with function object by reference", "[view filter]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v = viewify(vec);
    auto filter_fn = [](int x) { return x % 2 == 0; };
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
    view_assert_multipass_bidirectional(views::filter(v, filter_fn), {2, 4});
    view_assert_multipass_bidirectional(v | views::filter(filter_fn), {2, 4});
}
*/

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
        res, {range{v.forward_iter(), v.backward_iter()}}, view_ref_equals);
}
