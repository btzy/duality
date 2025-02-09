// This file is part of https://github.com/btzy/duality

#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <duality/actions/reverse.hpp>
#include <duality/viewifiers/contiguous.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("reverse action", "[action reverse]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v = viewify(vec);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
    SECTION("from view with function call syntax") {
        view_assert_random_access_bidirectional(actions::reverse(v), {5, 4, 3, 2, 1});
        view_assert_random_access_bidirectional(v, {5, 4, 3, 2, 1});
        CHECK_THAT(vec, Catch::Matchers::RangeEquals({5, 4, 3, 2, 1}));
    }
    /*
    SECTION("from view without adaptor") {
        view_assert_random_access_bidirectional(v | actions::reverse, {5, 4, 3, 2, 1});
        view_assert_random_access_bidirectional(v, {5, 4, 3, 2, 1});
        CHECK_THAT(vec, Catch::Matchers::RangeEquals({5, 4, 3, 2, 1}));
    }
    SECTION("from view with adaptor") {
        view_assert_random_access_bidirectional(v | actions::reverse(), {5, 4, 3, 2, 1});
        view_assert_random_access_bidirectional(v, {5, 4, 3, 2, 1});
        CHECK_THAT(vec, Catch::Matchers::RangeEquals({5, 4, 3, 2, 1}));
    }
    SECTION("from vector without adaptor") {
        view_assert_random_access_bidirectional(vec | actions::reverse, {5, 4, 3, 2, 1});
        view_assert_random_access_bidirectional(v, {5, 4, 3, 2, 1});
        CHECK_THAT(vec, Catch::Matchers::RangeEquals({5, 4, 3, 2, 1}));
    }
    SECTION("from vector with adaptor") {
        view_assert_random_access_bidirectional(vec | actions::reverse(), {5, 4, 3, 2, 1});
        view_assert_random_access_bidirectional(v, {5, 4, 3, 2, 1});
        CHECK_THAT(vec, Catch::Matchers::RangeEquals({5, 4, 3, 2, 1}));
    }
    */
}
