// This file is part of https://github.com/btzy/duality

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <duality/viewifiers/vector.hpp>
#include <duality/views/reverse.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("reverse view of finite_random_access_view", "[view reverse]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v = viewify(vec);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
    view_assert_finite_random_access(views::reverse(v), {5, 4, 3, 2, 1});
    view_assert_finite_random_access(v | views::reverse, {5, 4, 3, 2, 1});
    view_assert_finite_random_access(vec | views::reverse, {5, 4, 3, 2, 1});
    view_assert_finite_random_access(vec | views::reverse(), {5, 4, 3, 2, 1});
    view_assert_finite_random_access(std::vector<int>{1, 2, 3, 4, 5} | views::reverse(),
                                     {5, 4, 3, 2, 1});
    view_assert_finite_random_access(vec | views::reverse | views::reverse, {1, 2, 3, 4, 5});
}
