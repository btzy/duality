// This file is part of https://github.com/btzy/duality

#include <list>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/repeat.hpp>
#include <duality/viewifiers/forward_list.hpp>
#include <duality/viewifiers/vector.hpp>
#include <duality/views/filter.hpp>
#include "../view_assert.hpp"

using namespace duality;

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

TEST_CASE("filter view of infinite view is infinite", "[view filter]") {
    auto v = factories::repeat(5);
    auto res = v | views::filter([](int x) { return x % 2 == 0; });
    CHECK_FALSE(res.empty());
    auto size = res.size();
    static_assert(std::same_as<decltype(size), infinite_t>);
}
