// This file is part of https://github.com/btzy/duality

#include <forward_list>

#include <duality/viewifiers/forward_list.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("forward_list viewify", "[viewify forward_list]") {
    std::forward_list<int> list{1, 2, 3, 4, 5};
    auto lvalue = viewify(list);
    static_assert(std::same_as<view_element_type_t<decltype(lvalue)>, int&>);
    view_assert_multipass_forward(lvalue, {1, 2, 3, 4, 5});
    auto xvalue = viewify(std::move(list));
    static_assert(std::same_as<view_element_type_t<decltype(xvalue)>, int&>);
    view_assert_multipass_forward(xvalue, {1, 2, 3, 4, 5});
    auto prvalue = viewify(std::forward_list<int>{1, 2, 3, 4, 5});
    static_assert(std::same_as<view_element_type_t<decltype(prvalue)>, int&>);
    view_assert_multipass_forward(viewify(std::forward_list<int>{1, 2, 3, 4, 5}), {1, 2, 3, 4, 5});
}
