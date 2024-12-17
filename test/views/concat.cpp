// This file is part of https://github.com/btzy/duality

#include <list>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/repeat.hpp>
#include <duality/viewifiers/forward_list.hpp>
#include <duality/viewifiers/vector.hpp>
#include <duality/views/as_const.hpp>
#include <duality/views/concat.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("concat view", "[view concat]") {
    std::vector<int> vec1{1, 2, 3, 4, 5};
    std::vector<int> vec2{101, 102, 103, 104, 105};
    std::forward_list<int> flist{9, 8, 7};
    auto rep = factories::repeat(88);

    SECTION("one vector") {
        auto v1 = viewify(vec1);
        auto v = views::concat(v1);
        static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
        view_assert_random_access_bidirectional(v, {1, 2, 3, 4, 5});
    }

    SECTION("one forward_list") {
        auto v1 = viewify(flist);
        auto v = views::concat(v1);
        static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
        view_assert_multipass_forward(v, {9, 8, 7});
    }

    SECTION("one infinite") {
        auto v = views::concat(rep);
        static_assert(std::same_as<view_element_type_t<decltype(v)>, const int&>);
        view_assert_infinite_random_access_forward(v, {88, 88, 88, 88, 88});
    }

    SECTION("two vectors") {
        auto v1 = viewify(vec1);
        auto v2 = viewify(vec2);
        auto v = views::concat(v1, v2);
        static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
        view_assert_random_access_bidirectional(v, {1, 2, 3, 4, 5, 101, 102, 103, 104, 105});
    }

    SECTION("two vectors and one forward_list") {
        auto v1 = viewify(vec1);
        auto v2 = viewify(vec2);
        auto v3 = viewify(flist);
        auto v = views::concat(v1, v2, v3);
        static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
        view_assert_multipass_forward(v, {1, 2, 3, 4, 5, 101, 102, 103, 104, 105, 9, 8, 7});
    }

    SECTION("one forward_list and two vectors") {
        auto v1 = viewify(flist);
        auto v2 = viewify(vec1);
        auto v3 = viewify(vec2);
        auto v = views::concat(v1, v2, v3);
        static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
        view_assert_multipass_forward(v, {9, 8, 7, 1, 2, 3, 4, 5, 101, 102, 103, 104, 105});
    }

    SECTION("vector, forward_list, vector") {
        auto v1 = viewify(vec1);
        auto v2 = viewify(flist);
        auto v3 = viewify(vec2);
        auto v = views::concat(v1, v2, v3);
        static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
        view_assert_multipass_forward(v, {1, 2, 3, 4, 5, 9, 8, 7, 101, 102, 103, 104, 105});
    }

    SECTION("vector, infinite, vector") {
        auto v1 = viewify(vec1);
        auto v3 = viewify(vec2);
        auto v = views::concat(v1 | views::as_const, rep, v3 | views::as_const);
        static_assert(std::same_as<view_element_type_t<decltype(v)>, const int&>);
        view_assert_infinite_random_access_forward(v, {1, 2, 3, 4, 5, 88, 88, 88});
    }

    SECTION("vector, infinite, forward_list") {
        auto v1 = viewify(vec1);
        auto v3 = viewify(flist);
        auto v = views::concat(v1 | views::as_const, rep, v3 | views::as_const);
        static_assert(std::same_as<view_element_type_t<decltype(v)>, const int&>);
        view_assert_infinite_random_access_forward(v, {1, 2, 3, 4, 5, 88, 88, 88});
    }
}
