// This file is part of https://github.com/btzy/duality
#pragma once

#include <type_traits>
#include <utility>
#include <vector>

#include <duality/core_view.hpp>

#include <catch2/catch_test_macros.hpp>

template <typename V>
inline void view_assert_forward(
    V&& v,
    const std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>& expected) {
    // forward check
    {
        auto it = expected.begin();
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        while (auto opt = fit.next(rit)) {
            REQUIRE(it != expected.end());
            CHECK(*opt == *it++);
        }
        CHECK(it == expected.end());
    }

    {
        auto it = expected.begin();
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        while (fit.skip(rit)) {
            REQUIRE(it != expected.end());
            ++it;
        }
        CHECK(it == expected.end());
    }

    {
        auto fit = v.forward_iter();
        for (auto&& x : expected) {
            REQUIRE(fit.next() == x);
        }
        CHECK_FALSE(fit.next(v.backward_iter()));
    }

    {
        auto fit = v.forward_iter();
        for (auto&& x : expected) {
            (void)x;
            fit.skip();
        }
        CHECK_FALSE(fit.skip(v.backward_iter()));
    }
}

template <typename V>
inline void view_assert_forward_infinite(
    V&& v,
    const std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>& expected) {
    // forward check
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        for (auto&& x : expected) {
            auto opt = fit.next(rit);
            REQUIRE(opt);
            CHECK(*opt == x);
        }
        CHECK(fit.next(rit));
    }

    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        for (auto&& x : expected) {
            (void)x;
            REQUIRE(fit.skip(rit));
        }
        CHECK(fit.next(rit));
    }

    {
        auto fit = v.forward_iter();
        for (auto&& x : expected) {
            REQUIRE(fit.next() == x);
        }
        CHECK(fit.next(v.backward_iter()));
    }

    {
        auto fit = v.forward_iter();
        for (auto&& x : expected) {
            (void)x;
            fit.skip();
        }
        CHECK(fit.skip(v.backward_iter()));
    }
}

template <typename V>
inline void view_assert_backward(
    V&& v,
    const std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>& expected) {
    // backward check
    {
        auto it = expected.rbegin();
        const auto fit = v.forward_iter();
        auto rit = v.backward_iter();
        while (auto opt = rit.next(fit)) {
            REQUIRE(it != expected.rend());
            CHECK(*opt == *it++);
        }
        CHECK(it == expected.rend());
    }

    {
        auto it = expected.rbegin();
        const auto fit = v.forward_iter();
        auto rit = v.backward_iter();
        while (rit.skip(fit)) {
            REQUIRE(it != expected.rend());
            ++it;
        }
        CHECK(it == expected.rend());
    }

    {
        auto fit = v.backward_iter();
        for (auto it = expected.rbegin(); it != expected.rend(); ++it) {
            REQUIRE(fit.next() == *it);
        }
        CHECK_FALSE(fit.next(v.forward_iter()));
    }

    {
        auto fit = v.backward_iter();
        for (auto&& x : expected) {
            (void)x;
            fit.skip();
        }
        CHECK_FALSE(fit.skip(v.forward_iter()));
    }
}

template <typename V>
inline void view_assert_bidirectional(
    V&& v,
    const std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>& expected) {
    view_assert_forward(v, expected);
    view_assert_backward(v, expected);

    // bidirectional check
    auto it_begin = expected.begin();
    auto it_end = expected.end();
    auto fit = v.forward_iter();
    auto rit = v.backward_iter();
    bool from_back = false;
    while (true) {
        if (!from_back) {
            if (auto opt = fit.next(std::as_const(rit))) {
                REQUIRE(it_begin != it_end);
                CHECK(*opt == *it_begin++);
            } else {
                break;
            }
        } else {
            if (auto opt = rit.next(std::as_const(fit))) {
                REQUIRE(it_begin != it_end);
                CHECK(*opt == *--it_end);
            } else {
                break;
            }
        }
    }
    CHECK(it_begin == it_end);
}

template <typename DualityIter, typename DualityEndIter, typename StdIter>
inline void check_inverted(DualityIter fit, const DualityEndIter rit, StdIter curr, StdIter from) {
    for (size_t i = 0; i != 2; ++i) {
        if (auto opt = fit.next(rit)) {
            REQUIRE(curr != from);
            CHECK(*--curr == *opt);
        } else {
            CHECK(curr == from);
            break;
        }
    }
}

template <typename V>
inline void view_assert_multipass_bidirectional(
    V&& v,
    const std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>& expected) {
    view_assert_bidirectional(v, expected);

    // invertibility check (forward)
    {
        auto it = expected.begin();
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        check_inverted(fit.invert(), fit, it, expected.begin());
        while (auto opt = fit.next(rit)) {
            REQUIRE(it != expected.end());
            CHECK(*opt == *it++);
            check_inverted(fit.invert(), v.forward_iter(), it, expected.begin());
        }
        CHECK(it == expected.end());
    }

    // invertibility check (backward)
    {
        auto it = expected.rbegin();
        const auto fit = v.forward_iter();
        auto rit = v.backward_iter();
        check_inverted(rit.invert(), rit, it, expected.rbegin());
        while (auto opt = rit.next(fit)) {
            REQUIRE(it != expected.rend());
            CHECK(*opt == *it++);
            check_inverted(rit.invert(), v.backward_iter(), it, expected.rbegin());
        }
        CHECK(it == expected.rend());
    }
}

template <typename V>
inline void view_assert_multipass_forward(
    V&& v,
    const std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>& expected) {
    view_assert_forward(v, expected);

    // TODO check multipass property.
}

template <typename V>
inline void view_assert_emptyness(
    V&& v,
    const std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>& expected) {
    // emptyness check
    CHECK(v.empty() == expected.empty());
}

template <typename V>
inline void view_assert_sized(
    V&& v,
    const std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>& expected) {
    view_assert_emptyness(v, expected);

    // size check
    CHECK(v.size() == expected.size());
}

/// Checks if `v` as an infinite_random_access_view starts with the elements in `expected`.  The
/// view of course has more elements than `expected`, so we only check that the first
/// `expected.size()` elements match expect more elements to be consumable.
template <typename V>
inline void view_assert_infinite_multipass_forward(
    V&& v,
    const std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>& expected) {
    view_assert_forward_infinite(v, expected);

    // invertibility check (forward)
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        for (auto it = expected.begin(); it != expected.end(); ++it) {
            check_inverted(fit.invert(), v.forward_iter(), it, expected.begin());
            auto opt = fit.next(rit);
            REQUIRE(opt);
            CHECK(*opt == *it);
        }
        check_inverted(fit.invert(), v.forward_iter(), expected.end(), expected.begin());
    }
}

/// Checks if `v` as an infinite_random_access_view starts with the elements in `expected`.  The
/// view of course has more elements than `expected`, so we only check that the first
/// `expected.size()` elements match expect more elements to be consumable.
template <typename V>
inline void view_assert_infinite_random_access(
    V&& v,
    const std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>& expected) {
    view_assert_infinite_multipass_forward(v, expected);

    // indexing check
    for (size_t i = 0; i != expected.size(); ++i) {
        CHECK(v[i] == expected[i]);
    }

    // forward skip(n, it)
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        CHECK(fit.skip(0, rit) == 0);
        CHECK(fit.skip(half, rit) == half);
        CHECK(fit.skip(expected.size() - half, rit) == expected.size() - half);
        CHECK(fit.skip(rit));
    }

    // forward skip(n)
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        fit.skip(0);
        fit.skip(half);
        fit.skip(expected.size() - half);
        CHECK(fit.skip(rit));
    }
}

template <typename V>
inline void view_assert_finite_random_access(
    V&& v,
    const std::vector<std::remove_cvref_t<duality::view_element_type_t<V>>>& expected) {
    view_assert_multipass_bidirectional(v, expected);
    view_assert_sized(v, expected);

    // indexing check
    for (size_t i = 0; i != expected.size(); ++i) {
        CHECK(v[i] == expected[i]);
    }

    // forward skip(n, it)
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        CHECK(fit.skip(0, rit) == 0);
        CHECK(fit.skip(half, rit) == half);
        CHECK(fit.skip(expected.size() - half, rit) == expected.size() - half);
        CHECK_FALSE(fit.skip(rit));
    }

    // forward skip(n)
    {
        auto fit = v.forward_iter();
        const auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        fit.skip(0);
        fit.skip(half);
        fit.skip(expected.size() - half);
        CHECK_FALSE(fit.skip(rit));
    }

    // backward skip(n, it)
    {
        const auto fit = v.forward_iter();
        auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        CHECK(rit.skip(0, fit) == 0);
        CHECK(rit.skip(half, fit) == half);
        CHECK(rit.skip(expected.size() - half, fit) == expected.size() - half);
        CHECK_FALSE(rit.skip(fit));
    }

    // backward skip(n)
    {
        const auto fit = v.forward_iter();
        auto rit = v.backward_iter();
        const size_t half = expected.size() / 2;
        rit.skip(0);
        rit.skip(half);
        rit.skip(expected.size() - half);
        CHECK_FALSE(rit.skip(fit));
    }
}
