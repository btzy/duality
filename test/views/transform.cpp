// This file is part of https://github.com/btzy/duality

#include <list>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/empty.hpp>
#include <duality/factories/single.hpp>
#include <duality/viewifiers/forward_list.hpp>
#include <duality/viewifiers/vector.hpp>
#include <duality/views/transform.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("finite transform view of random_access_bidirectional_view", "[view transform]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v = viewify(vec);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
    view_assert_random_access_bidirectional(views::transform(v, [](int x) { return x * 2; }),
                                            {2, 4, 6, 8, 10});
    view_assert_random_access_bidirectional(v | views::transform([](int x) { return x * 2; }),
                                            {2, 4, 6, 8, 10});
    view_assert_random_access_bidirectional(vec | views::transform([](int x) { return x * 2; }),
                                            {2, 4, 6, 8, 10});
    view_assert_random_access_bidirectional(
        std::vector<int>{1, 2, 3, 4, 5} | views::transform([](int x) { return x * 2; }),
        {2, 4, 6, 8, 10});
    view_assert_random_access_bidirectional(vec | views::transform([](int x) { return x * 2; }) |
                                                views::transform([](int x) { return x * 3; }),
                                            {6, 12, 18, 24, 30});

    view_assert_random_access_bidirectional(factories::empty<int>() | views::transform([](int x) {
                                                return x * 2;
                                            }) | views::transform([](int x) { return x * 3; }),
                                            {});
    view_assert_random_access_bidirectional(factories::single(100) | views::transform([](int x) {
                                                return x * 2;
                                            }) | views::transform([](int x) { return x * 3; }),
                                            {600});
}

TEST_CASE("finite transform view of multipass_forward_view", "[view transform]") {
    std::forward_list<int> flist{1, 2, 3, 4, 5};
    auto v = viewify(flist);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
    view_assert_multipass_forward(views::transform(v, [](int x) { return x * 2; }),
                                  {2, 4, 6, 8, 10});
    view_assert_multipass_forward(v | views::transform([](int x) { return x * 2; }),
                                  {2, 4, 6, 8, 10});
    view_assert_multipass_forward(flist | views::transform([](int x) { return x * 2; }),
                                  {2, 4, 6, 8, 10});
    view_assert_multipass_forward(
        std::vector<int>{1, 2, 3, 4, 5} | views::transform([](int x) { return x * 2; }),
        {2, 4, 6, 8, 10});
    view_assert_multipass_forward(flist | views::transform([](int x) { return x * 2; }) |
                                      views::transform([](int x) { return x * 3; }),
                                  {6, 12, 18, 24, 30});
}

TEST_CASE("transform view with function object by reference", "[view transform]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v = viewify(vec);
    auto transform_fn = [](int x) { return x * 2; };
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
    view_assert_random_access_bidirectional(views::transform(v, transform_fn), {2, 4, 6, 8, 10});
    view_assert_random_access_bidirectional(v | views::transform(transform_fn), {2, 4, 6, 8, 10});
}
