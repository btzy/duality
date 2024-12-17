// This file is part of https://github.com/btzy/duality

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/repeat.hpp>
#include <duality/viewifiers/forward_list.hpp>
#include <duality/viewifiers/vector.hpp>
#include <duality/views/filter.hpp>
#include <duality/views/reverse.hpp>
#include <duality/views/transform.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("finite mixed views", "[view mixed]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::forward_list<int> flist{1, 2, 3, 4, 5};

    // vector
    view_assert_random_access_bidirectional(
        vec | views::reverse | views::transform([](int x) { return x * 2; }), {10, 8, 6, 4, 2});
    view_assert_random_access_bidirectional(
        vec | views::transform([](int x) { return x * 2; }) | views::reverse, {10, 8, 6, 4, 2});
    view_assert_multipass_bidirectional(vec | views::transform([](int x) { return x * 2; }) |
                                            views::filter([](int x) { return x > 5; }),
                                        {6, 8, 10});
    view_assert_multipass_bidirectional(vec | views::filter([](int x) { return x % 2 == 0; }) |
                                            views::transform([](int x) { return x * 5; }),
                                        {10, 20});
    view_assert_multipass_bidirectional(
        vec | views::reverse | views::filter([](int x) { return x % 2 == 0; }) | views::reverse,
        {2, 4});

    // forward_list
    view_assert_multipass_forward(flist | views::transform([](int x) { return x * 2; }) |
                                      views::filter([](int x) { return x > 5; }),
                                  {6, 8, 10});
    view_assert_multipass_forward(flist | views::filter([](int x) { return x % 2 == 0; }) |
                                      views::transform([](int x) { return x * 5; }),
                                  {10, 20});
}

TEST_CASE("infinite mixed views", "[view mixed]") {
    auto v = factories::repeat(123);
    view_assert_infinite_random_access_forward(v | views::transform([](int x) { return x * 2; }),
                                               {246, 246, 246});
    view_assert_infinite_random_access_forward(
        factories::repeat(123) | views::transform([](int x) { return x * 2; }), {246, 246, 246});
    view_assert_infinite_multipass_forward(v | views::transform([](int x) { return x * 2; }) |
                                               views::filter([](int x) { return x > 100; }),
                                           {246, 246, 246});
}
