// This file is part of https://github.com/btzy/duality

#include <list>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <duality/factories/empty.hpp>
#include <duality/factories/iota.hpp>
#include <duality/factories/single.hpp>
#include <duality/viewifiers/forward_list.hpp>
#include <duality/viewifiers/vector.hpp>
#include <duality/views/lazy_take.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("finite lazy_take view of finite_random_access_view", "[view lazy_take]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v = viewify(vec);
    static_assert(std::same_as<view_element_type_t<decltype(v)>, int&>);
    view_assert_multipass_forward(views::lazy_take(v, 3), {1, 2, 3});
    view_assert_multipass_forward(v | views::lazy_take(3), {1, 2, 3});
    view_assert_multipass_forward(std::vector<int>{1, 2, 3, 4, 5} | views::lazy_take(3), {1, 2, 3});
    view_assert_emptyness(std::vector<int>{1, 2, 3, 4, 5} | views::lazy_take(3), {1, 2, 3});
    view_assert_sized(std::vector<int>{1, 2, 3, 4, 5} | views::lazy_take(3), {1, 2, 3});
    view_assert_multipass_forward(std::vector<int>{1, 2, 3, 4, 5} | views::lazy_take(8),
                                  {1, 2, 3, 4, 5});
    view_assert_emptyness(std::vector<int>{1, 2, 3, 4, 5} | views::lazy_take(8), {1, 2, 3, 4, 5});
    view_assert_sized(std::vector<int>{1, 2, 3, 4, 5} | views::lazy_take(8), {1, 2, 3, 4, 5});
    view_assert_multipass_forward(std::vector<int>{1, 2, 3, 4, 5} | views::lazy_take(0), {});
    view_assert_emptyness(std::vector<int>{1, 2, 3, 4, 5} | views::lazy_take(0), {});
    view_assert_sized(std::vector<int>{1, 2, 3, 4, 5} | views::lazy_take(0), {});
    view_assert_multipass_forward(factories::empty<int>() | views::lazy_take(3), {});
    view_assert_emptyness(factories::empty<int>() | views::lazy_take(3), {});
    view_assert_sized(factories::empty<int>() | views::lazy_take(3), {});
    view_assert_multipass_forward(factories::empty<int>() | views::lazy_take(0), {});
    view_assert_emptyness(factories::empty<int>() | views::lazy_take(0), {});
    view_assert_sized(factories::empty<int>() | views::lazy_take(0), {});
    view_assert_multipass_forward(factories::single(100) | views::lazy_take(3), {100});
    view_assert_emptyness(factories::single(100) | views::lazy_take(3), {100});
    view_assert_sized(factories::single(100) | views::lazy_take(3), {100});
    view_assert_multipass_forward(factories::single(100) | views::lazy_take(0), {});
    view_assert_emptyness(factories::single(100) | views::lazy_take(0), {});
    view_assert_sized(factories::single(100) | views::lazy_take(0), {});
}

TEST_CASE("finite lazy_take view of multipass_forward_view", "[view lazy_take]") {
    view_assert_multipass_forward(std::forward_list<int>{1, 2, 3, 4, 5} | views::lazy_take(3),
                                  {1, 2, 3});
    view_assert_emptyness(std::forward_list<int>{1, 2, 3, 4, 5} | views::lazy_take(3), {1, 2, 3});
    view_assert_multipass_forward(std::forward_list<int>{1, 2, 3, 4, 5} | views::lazy_take(8),
                                  {1, 2, 3, 4, 5});
    view_assert_emptyness(std::forward_list<int>{1, 2, 3, 4, 5} | views::lazy_take(8),
                          {1, 2, 3, 4, 5});
    view_assert_multipass_forward(std::forward_list<int>{1, 2, 3, 4, 5} | views::lazy_take(0), {});
    view_assert_emptyness(std::forward_list<int>{1, 2, 3, 4, 5} | views::lazy_take(0), {});
}

TEST_CASE("finite lazy_take view of infinite_random_access_view", "[view lazy_take]") {
    view_assert_multipass_forward(factories::iota(0) | views::lazy_take(3), {0, 1, 2});
    view_assert_emptyness(factories::iota(0) | views::lazy_take(3), {0, 1, 2});
    view_assert_multipass_forward(factories::iota(0) | views::lazy_take(0), {});
    view_assert_emptyness(factories::iota(0) | views::lazy_take(0), {});
}
