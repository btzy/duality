// This file is part of https://github.com/btzy/duality

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <duality/viewifiers/vector.hpp>
#include <duality/views/as_const.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("as_const view", "[view as_const]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v1 = viewify(vec) | views::as_const;
    auto v2 = viewify(vec) | views::as_const | views::as_const;
    auto v3 = viewify(vec) | views::as_const | views::as_const | views::as_const;
    auto v4 = v1 | views::as_const;
    static_assert(std::same_as<view_element_type_t<decltype(v1)>, const int&>);
    static_assert(std::same_as<view_element_type_t<decltype(v2)>, const int&>);
    static_assert(std::same_as<view_element_type_t<decltype(v3)>, const int&>);
    static_assert(std::same_as<view_element_type_t<decltype(v4)>, const int&>);
    view_assert_random_access_bidirectional(v1, {1, 2, 3, 4, 5});
    view_assert_random_access_bidirectional(v2, {1, 2, 3, 4, 5});
    view_assert_random_access_bidirectional(v3, {1, 2, 3, 4, 5});
    view_assert_random_access_bidirectional(v4, {1, 2, 3, 4, 5});
    // check that passing an already-const view throug as_const returns the same view.
    static_assert(std::same_as<decltype(v1), decltype(v2)>);
    static_assert(std::same_as<decltype(v2), decltype(v3)>);
    static_assert(std::same_as<decltype(v3), decltype(v4)>);
}
