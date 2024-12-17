// This file is part of https://github.com/btzy/duality

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/iota.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("infinite iota view", "[view iota]") {
    auto v = factories::iota(static_cast<size_t>(5));
    static_assert(std::same_as<view_element_type_t<decltype(v)>, size_t>);
    view_assert_infinite_random_access_forward(v, {5, 6, 7, 8, 9, 10, 11, 12, 13, 14});
}

TEST_CASE("finite iota view", "[view iota]") {
    auto v = factories::iota(static_cast<size_t>(5), static_cast<size_t>(8));
    static_assert(std::same_as<view_element_type_t<decltype(v)>, size_t>);
    view_assert_random_access_bidirectional(v, {5, 6, 7});
}
