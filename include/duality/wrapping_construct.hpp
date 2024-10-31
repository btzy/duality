// This file is part of https://github.com/btzy/duality
#pragma once

/// This file contains the wrapping_construct_t type.

namespace duality {

/// This is a tag type for constructing an iterator from another iterator or a view from another
/// view.  It disambiguates constructing a view or iterator that wraps another from performing copy
/// construction when we use CTAD.
struct wrapping_construct_t {};
constexpr wrapping_construct_t wrapping_construct{};

}  // namespace duality
