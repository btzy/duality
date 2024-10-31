// This file is part of https://github.com/btzy/duality

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/repeat.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("infinite repeat view", "[view repeat]") {
    auto v = factories::repeat(5);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, const int&>);
    view_assert_infinite_random_access(v, {5, 5, 5, 5, 5, 5, 5, 5, 5, 5});

    int x = 5;
    auto vref = factories::repeat(x);
    static_assert(std::same_as<view_element_type_t<decltype(vref)>, const int&>);
    view_assert_infinite_random_access(vref, {5, 5, 5, 5, 5, 5, 5, 5, 5, 5});
    for (size_t i = 0; i != 4; ++i) {
        CHECK(&vref[i] == &x);
    }
    CHECK(&vref.forward_iter().next() == &x);
}

TEST_CASE("finite repeat view", "[view repeat]") {
    auto v = factories::repeat(5, static_cast<size_t>(8));
    static_assert(std::same_as<view_element_type_t<decltype(v)>, const int&>);
    view_assert_finite_random_access(v, {5, 5, 5, 5, 5, 5, 5, 5});

    int x = 5;
    auto vref = factories::repeat(x, static_cast<size_t>(8));
    static_assert(std::same_as<view_element_type_t<decltype(vref)>, const int&>);
    view_assert_finite_random_access(vref, {5, 5, 5, 5, 5, 5, 5, 5});
    for (size_t i = 0; i != 8; ++i) {
        CHECK(&vref[i] == &x);
    }
    CHECK(&vref.forward_iter().next() == &x);
    CHECK(&vref.backward_iter().next() == &x);
}
