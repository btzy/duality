set(TEST_LIST
  view_assert.hpp
  actions/reverse.cpp
  factories/iota.cpp
  factories/repeat.cpp
  factories/single.cpp
  # operations/to.cpp
  viewifiers/contiguous.cpp
  viewifiers/forward_list.cpp
  viewifiers/istream.cpp
  views/as_const.cpp
  views/as_input_range.cpp
  # views/concat.cpp # problem because concat is not currently implemented for backward views
  views/eager_split_by.cpp
  # views/eager_take.cpp
  views/filter.cpp
  views/lazy_take.cpp
  views/mixed.cpp
  # views/range_based_for_loop.cpp
  views/reverse.cpp
  views/transform.cpp
  )

add_executable(unit_test ${TEST_LIST})
target_link_libraries(unit_test PRIVATE duality)
target_link_libraries(unit_test PRIVATE Catch2::Catch2WithMain)
