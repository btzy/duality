// This file is part of https://github.com/btzy/duality

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/repeat.hpp>
#include "../view_assert.hpp"

using namespace duality;

namespace {

template <typename V, typename T>
void check_reference_forward(V&& v, size_t size, T& t) {
    auto fit = v.forward_iter();
    for (size_t i = 0; i != size; ++i) {
        const auto& elem = fit.next();
        CHECK(&elem == &t);
    }
}

template <typename V, typename T>
void check_reference_backward(V&& v, size_t size, T& t) {
    auto fit = v.backward_iter();
    for (size_t i = 0; i != size; ++i) {
        const auto& elem = fit.next();
        CHECK(&elem == &t);
    }
}

}  // namespace

TEST_CASE("infinite repeat view", "[view repeat]") {
    auto v = factories::repeat(5);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, const int&>);
    view_assert_infinite_random_access_forward(v, {5, 5, 5, 5, 5, 5, 5, 5, 5, 5});
    view_assert_infinite_random_access_backward(v, {5, 5, 5, 5, 5, 5, 5, 5, 5, 5});

    int x = 5;
    auto vref = factories::repeat(x);
    static_assert(std::same_as<view_element_type_t<decltype(vref)>, const int&>);
    view_assert_infinite_random_access_forward(vref, {5, 5, 5, 5, 5, 5, 5, 5, 5, 5});
    view_assert_infinite_random_access_backward(vref, {5, 5, 5, 5, 5, 5, 5, 5, 5, 5});
    check_reference_forward(vref, 4, x);
    check_reference_backward(vref, 4, x);
}

TEST_CASE("finite repeat view", "[view repeat]") {
    auto v = factories::repeat(5, static_cast<size_t>(8));
    static_assert(std::same_as<view_element_type_t<decltype(v)>, const int&>);
    view_assert_random_access_bidirectional(v, {5, 5, 5, 5, 5, 5, 5, 5});

    int x = 5;
    auto vref = factories::repeat(x, static_cast<size_t>(8));
    static_assert(std::same_as<view_element_type_t<decltype(vref)>, const int&>);
    view_assert_random_access_bidirectional(vref, {5, 5, 5, 5, 5, 5, 5, 5});
    check_reference_forward(vref, 8, x);
    check_reference_backward(vref, 8, x);
}
