// This file is part of https://github.com/btzy/duality

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/iota.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("infinite iota view", "[view iota]") {
    auto v = factories::iota(static_cast<size_t>(5));
    static_assert(std::same_as<view_element_type_t<decltype(v)>, size_t>);
    view_assert_infinite_random_access(v, {5, 6, 7, 8, 9, 10, 11, 12, 13, 14});
}

TEST_CASE("infinite iota view with smaller signed int", "[view iota]") {
    auto v = factories::iota(5);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int>);

    CHECK(v[0] == 5);
    CHECK(v[1] == 6);
    CHECK(v[2] == 7);
    CHECK(v[1000000] == 1000005);
    auto fit = v.forward_iter();
    CHECK(fit.next() == 5);
    CHECK(fit.next() == 6);
    CHECK(fit.next() == 7);
    CHECK(fit.next() == 8);
    CHECK(fit.next() == 9);
}

TEST_CASE("finite iota view", "[view iota]") {
    auto v = factories::iota(static_cast<size_t>(5), static_cast<size_t>(8));
    static_assert(std::same_as<view_element_type_t<decltype(v)>, size_t>);
    view_assert_finite_random_access(v, {5, 6, 7});
}

TEST_CASE("finite iota view with smaller signed int", "[view iota]") {
    auto v = factories::iota(5, 8);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int>);

    REQUIRE(v.size() == 3);
    CHECK(v[0] == 5);
    CHECK(v[1] == 6);
    CHECK(v[2] == 7);

    auto fit = v.forward_iter();
    {
        auto opt = fit.next(v.backward_iter());
        REQUIRE(opt);
        CHECK(*opt == 5);
    }
    {
        auto opt = fit.next(v.backward_iter());
        REQUIRE(opt);
        CHECK(*opt == 6);
    }
    {
        auto opt = fit.next(v.backward_iter());
        REQUIRE(opt);
        CHECK(*opt == 7);
    }
    {
        auto opt = fit.next(v.backward_iter());
        REQUIRE(!opt);
    }

    auto rit = v.backward_iter();
    {
        auto opt = rit.next(v.forward_iter());
        REQUIRE(opt);
        CHECK(*opt == 7);
    }
    {
        auto opt = rit.next(v.forward_iter());
        REQUIRE(opt);
        CHECK(*opt == 6);
    }
    {
        auto opt = rit.next(v.forward_iter());
        REQUIRE(opt);
        CHECK(*opt == 5);
    }
    {
        auto opt = rit.next(v.forward_iter());
        REQUIRE(!opt);
    }
}
