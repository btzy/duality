// This file is part of https://github.com/btzy/duality
#pragma once

// Stuff that converts a std::forward_list into a view.  When the container is passed by lvalue, it
// will old a reference to the other container.  If passed by rvalue, it will move the container
// into itself.

#include <istream>
#include <type_traits>

#include <duality/core_view.hpp>

namespace duality {

namespace impl {

template <typename Stream>
concept basic_istream_concept = std::is_object_v<Stream> && requires(Stream& s) {
    []<typename CharT, typename Traits>(std::basic_istream<CharT, Traits>&) {}(s);
};
template <typename Stream>
concept basic_istream_cvref_concept = basic_istream_concept<std::remove_cvref_t<Stream>>;
template <typename Stream>
concept basic_istream_cv_concept = basic_istream_concept<std::remove_cv_t<Stream>>;

}  // namespace impl

template <typename Val, impl::basic_istream_cvref_concept Stream>
class basic_istream_viewifier;

namespace impl {

class basic_istream_sentinel {};

/// @brief Represents a forward iterator of a basic_istream.  This type is movable, but moving it
/// leaves an object where the only valid operations are reassignment or destruction.
/// @tparam Val The type of value produced.
template <typename Val, impl::basic_istream_cv_concept Stream>
class basic_istream_forward_iterator {
   private:
    template <typename, basic_istream_cvref_concept>
    friend class duality::basic_istream_viewifier;
    Stream* stream_;
    using element_type = Val;

    constexpr basic_istream_forward_iterator(Stream& stream) noexcept : stream_(&stream) {}

   public:
    constexpr basic_istream_forward_iterator() noexcept : stream_(nullptr) {}
    constexpr basic_istream_forward_iterator(basic_istream_forward_iterator&& other) noexcept
        : stream_(std::exchange(other.stream_, nullptr)) {}
    constexpr basic_istream_forward_iterator& operator=(
        basic_istream_forward_iterator&& other) noexcept {
        stream_(std::exchange(other.stream_, nullptr));
    }

    using index_type = no_index_type_t;
    constexpr element_type next() noexcept {
        Val ret;
        *stream_ >> ret;
        return ret;
    }
    constexpr optional<Val> next(const basic_istream_sentinel&) noexcept {
        optional<Val> ret(in_place);
        if (!(*stream_ >> *ret)) {
            // change ret to an empty optional
            std::destroy_at(&ret);
            std::construct_at(&ret);
        }
        return ret;
    }
    constexpr void skip() noexcept {
        Val ret;
        *stream_ >> ret;
    }
    constexpr bool skip(const basic_istream_sentinel&) noexcept {
        Val ret;
        return static_cast<bool>(*stream_ >> ret);
    }
};

}  // namespace impl

/// @brief Represents a view that references a forward_list.
/// @tparam C The container type.  Should be a forward_list or const forward_list.
template <typename Val, impl::basic_istream_cvref_concept Stream>
class basic_istream_viewifier {
   private:
    [[no_unique_address]] Stream stream_;

   public:
    template <typename Stream2>
    constexpr basic_istream_viewifier(Stream2&& stream) noexcept
        : stream_(std::forward<Stream2>(stream)) {}
    constexpr auto forward_iter() const noexcept
        requires std::is_reference_v<Stream>
    {
        return impl::basic_istream_forward_iterator<Val, std::remove_reference_t<Stream>>(stream_);
    }
    constexpr auto forward_iter() noexcept
        requires(!std::is_reference_v<Stream>)
    {
        return impl::basic_istream_forward_iterator<Val, Stream>(stream_);
    }
    constexpr auto backward_iter() const noexcept { return impl::basic_istream_sentinel{}; }
};

/// @brief This function returns a wrapper for the given forward_list (by move).
/// @tparam T A forward_list type.
/// @param v The forward_list.
/// @return The wrapper.
template <typename Val, impl::basic_istream_cvref_concept T>
constexpr auto viewify(T&& t) {
    return basic_istream_viewifier<Val, T>(std::forward<T>(t));
}

}  // namespace duality
