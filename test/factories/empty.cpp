// This file is part of https://github.com/btzy/duality

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/empty.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("empty view", "[view empty]") {
    auto v = factories::empty<int>();
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int>);
    view_assert_random_access_bidirectional(v, {});
}
