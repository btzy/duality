// This file is part of https://github.com/btzy/duality

#include <list>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/empty.hpp>
#include <duality/factories/iota.hpp>
#include <duality/factories/repeat.hpp>
#include <duality/stage_views/join.hpp>
#include <duality/views/transform.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("forward_join stage view", "[stage_view forward_join]") {
    {
        auto triangle_view = factories::iota(static_cast<size_t>(1), static_cast<size_t>(6)) |
                             views::transform([](size_t x) {
                                 // need std::move to make iota take it by value
                                 return factories::iota(static_cast<size_t>(0), std::move(x));
                             });
        auto joined_stage_view = triangle_view | stage_views::join_forward();
        view_assert_forward_singlepass(
            [&] {
                auto stage = joined_stage_view.stage();
                static_assert(std::same_as<view_element_type_t<decltype(stage)>, size_t>);
                return stage;
            },
            {0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4});
    }
    {
        auto triangle_view = factories::iota(static_cast<size_t>(1), static_cast<size_t>(6)) |
                             views::transform([](size_t x) {
                                 // need std::move to make iota take it by value
                                 return factories::iota(static_cast<size_t>(0), std::move(x));
                             });
        auto joined_stage_view = std::move(triangle_view) | stage_views::join_forward();
        view_assert_forward_singlepass(
            [&] {
                auto stage = joined_stage_view.stage();
                static_assert(std::same_as<view_element_type_t<decltype(stage)>, size_t>);
                return stage;
            },
            {0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4});
    }
    {
        auto joined_stage_view = factories::iota(static_cast<size_t>(1), static_cast<size_t>(6)) |
                                 views::transform([](size_t x) {
                                     // need std::move to make iota take it by value
                                     return factories::iota(static_cast<size_t>(0), std::move(x));
                                 }) |
                                 stage_views::join_forward();
        view_assert_forward_singlepass(
            [&] {
                auto stage = joined_stage_view.stage();
                static_assert(std::same_as<view_element_type_t<decltype(stage)>, size_t>);
                return stage;
            },
            {0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4});
    }
    {
        auto joined_stage_view = factories::iota(static_cast<size_t>(1), static_cast<size_t>(6)) |
                                 views::transform([](size_t x) {
                                     // need std::move to make iota take it by value
                                     return factories::iota(static_cast<size_t>(0), std::move(x));
                                 }) |
                                 stage_views::join_forward();
        view_assert_forward_singlepass(
            [&] {
                auto stage = std::move(joined_stage_view).stage();
                static_assert(std::same_as<view_element_type_t<decltype(stage)>, size_t>);
                return stage;
            },
            {0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4});
    }
    {
        view_assert_forward_singlepass(
            [&] {
                auto stage = (factories::iota(static_cast<size_t>(1), static_cast<size_t>(6)) |
                              views::transform([](size_t x) {
                                  // need std::move to make iota take it by value
                                  return factories::iota(static_cast<size_t>(0), std::move(x));
                              }) |
                              stage_views::join_forward())
                                 .stage();
                static_assert(std::same_as<view_element_type_t<decltype(stage)>, size_t>);
                return stage;
            },
            {0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4});
    }
}

TEST_CASE("forward_join stage view with some empty subviews", "[stage_view forward_join]") {
    auto triangle_view = factories::iota(static_cast<size_t>(10), static_cast<size_t>(14)) |
                         views::transform([](size_t x) {
                             // need std::move to make iota take it by value
                             return factories::iota(static_cast<size_t>(0), x % 2) |
                                    views::transform([x](size_t y) { return y + x; });
                         });
    auto joined_stage_view = triangle_view | stage_views::join_forward();
    view_assert_forward_singlepass(
        [&] {
            auto stage = joined_stage_view.stage();
            static_assert(std::same_as<view_element_type_t<decltype(stage)>, size_t>);
            return stage;
        },
        {11, 13});
}

TEST_CASE("forward_join stage view with all empty subviews", "[stage_view forward_join]") {
    auto triangle_view = factories::repeat(static_cast<size_t>(7), static_cast<size_t>(4)) |
                         views::transform([](size_t) {
                             // need std::move to make iota take it by value
                             return factories::empty<size_t>();
                         });
    auto joined_stage_view = triangle_view | stage_views::join_forward();
    view_assert_forward_singlepass(
        [&] {
            auto stage = joined_stage_view.stage();
            static_assert(std::same_as<view_element_type_t<decltype(stage)>, size_t>);
            return stage;
        },
        {});
}

TEST_CASE("forward_join stage view empty", "[stage_view forward_join]") {
    auto triangle_view = factories::iota(static_cast<size_t>(10), static_cast<size_t>(10)) |
                         views::transform([](size_t x) {
                             // need std::move to make iota take it by value
                             return factories::iota(static_cast<size_t>(0), std::move(x));
                         });
    auto joined_stage_view = triangle_view | stage_views::join_forward();
    view_assert_forward_singlepass(
        [&] {
            auto stage = joined_stage_view.stage();
            static_assert(std::same_as<view_element_type_t<decltype(stage)>, size_t>);
            return stage;
        },
        {});
}
