// This file is part of https://github.com/btzy/duality

#include <vector>

#include <duality/viewifiers/contiguous.hpp>
#include "../view_assert.hpp"

using namespace duality;

TEST_CASE("contiguous container viewify", "[viewify contiguous_container]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto v_lvalue = viewify(vec);
    static_assert(std::same_as<view_element_type_t<decltype(v_lvalue)>, int&>);
    view_assert_random_access_bidirectional(v_lvalue, {1, 2, 3, 4, 5});
    auto v_xvalue = viewify(std::move(vec));
    static_assert(std::same_as<view_element_type_t<decltype(v_xvalue)>, int&>);
    view_assert_random_access_bidirectional(v_xvalue, {1, 2, 3, 4, 5});
    auto v_prvalue = viewify(std::vector<int>{1, 2, 3, 4, 5});
    static_assert(std::same_as<view_element_type_t<decltype(v_prvalue)>, int&>);
    view_assert_random_access_bidirectional(viewify(std::vector<int>{1, 2, 3, 4, 5}),
                                            {1, 2, 3, 4, 5});
}
