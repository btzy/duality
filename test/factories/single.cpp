// This file is part of https://github.com/btzy/duality

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/single.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("single view", "[view single]") {
    auto v = factories::single(123);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, const int&>);
    view_assert_finite_random_access(v, {123});

    size_t val = 321;
    auto vref = factories::single(val);
    static_assert(std::same_as<view_element_type_t<decltype(vref)>, const size_t&>);
    view_assert_finite_random_access(vref, {321});
    CHECK(&vref[0] == &val);
    CHECK(&vref.forward_iter().next() == &val);
    CHECK(&vref.backward_iter().next() == &val);
}
