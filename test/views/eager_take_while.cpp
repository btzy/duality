// This file is part of https://github.com/btzy/duality

#include <list>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/iota.hpp>
#include <duality/viewifiers/forward_list.hpp>
#include <duality/viewifiers/vector.hpp>
#include <duality/views/eager_take_while.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("finite eager_take_while view of random_access_bidirectional_view",
          "[view eager_take_while]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v = viewify(vec);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
    view_assert_random_access_bidirectional(
        views::eager_take_while(v, [](int x) { return x <= 3; }), {1, 2, 3});
    view_assert_random_access_bidirectional(
        v | views::eager_take_while([](int x) { return x <= 3; }), {1, 2, 3});
    view_assert_random_access_bidirectional(
        std::vector<int>{1, 2, 3, 4, 5} | views::eager_take_while([](int x) { return x <= 3; }),
        {1, 2, 3});
    view_assert_random_access_bidirectional(
        views::eager_take_while(v, [](int x) { return x % 2 == 1; }), {1});
    view_assert_random_access_bidirectional(
        views::eager_take_while(v, [](int x) { return x % 2 == 0; }), {});
}

TEST_CASE("finite eager_take_while view of multipass_forward_view", "[view eager_take_while]") {
    view_assert_multipass_forward(std::forward_list<int>{1, 2, 3, 4, 5} |
                                      views::eager_take_while([](int x) { return x <= 3; }),
                                  {1, 2, 3});
    view_assert_multipass_forward(std::forward_list<int>{1, 2, 3, 4, 5} |
                                      views::eager_take_while([](int x) { return x % 2 == 1; }),
                                  {1});
    view_assert_multipass_forward(std::forward_list<int>{1, 2, 3, 4, 5} |
                                      views::eager_take_while([](int x) { return x % 2 == 0; }),
                                  {});
}

TEST_CASE("finite eager_take_while view of infinite random_access_bidirectional_view",
          "[view eager_take_while]") {
    view_assert_random_access_bidirectional(
        factories::iota(static_cast<size_t>(0)) |
            views::eager_take_while([](size_t x) { return x <= 3; }),
        {0, 1, 2});
    view_assert_random_access_bidirectional(
        factories::iota(static_cast<size_t>(0)) |
            views::eager_take_while([](size_t x) { return x % 2 == 1; }),
        {});
    view_assert_random_access_bidirectional(
        factories::iota(static_cast<size_t>(0)) |
            views::eager_take_while([](size_t x) { return x % 2 == 0; }),
        {0});
    view_assert_random_access_bidirectional(
        factories::iota(static_cast<size_t>(0)) |
            views::eager_take_while([](size_t x) { return x / 10 % 10 == 0; }),
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
}

TEST_CASE("eager_take_while view with function object by reference", "[view eager_take_while]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v = viewify(vec);
    auto eager_take_while_fn = [](int x) { return x <= 3; };
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
    view_assert_random_access_bidirectional(views::eager_take_while(v, eager_take_while_fn),
                                            {1, 2, 3});
    view_assert_random_access_bidirectional(v | views::eager_take_while(eager_take_while_fn),
                                            {1, 2, 3});
    view_assert_random_access_bidirectional(
        std::vector<int>{1, 2, 3, 4, 5} | views::eager_take_while(eager_take_while_fn), {1, 2, 3});
}
