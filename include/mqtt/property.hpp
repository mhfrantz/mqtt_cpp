// Copyright Takatoshi Kondo 2018
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_PROPERTY_HPP)
#define MQTT_PROPERTY_HPP

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

#include <boost/asio/buffer.hpp>
#include <boost/optional.hpp>
#include <boost/container/static_vector.hpp>

#include <mqtt/two_byte_util.hpp>
#include <mqtt/fixed_header.hpp>
#include <mqtt/remaining_length.hpp>
#include <mqtt/qos.hpp>
#include <mqtt/const_buffer_util.hpp>
#include <mqtt/will.hpp>
#include <mqtt/connect_flags.hpp>
#include <mqtt/publish.hpp>
#include <mqtt/exception.hpp>

namespace mqtt {

namespace as = boost::asio;

namespace v5 {

namespace property {

namespace detail {

template <std::size_t N>
struct n_bytes_property {
    n_bytes_property(char id, char val)
        :id_(id), buf_(val) {}

    /**
     * @brief Create const buffer sequence
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        return
            {
                as::buffer(&id_, 1),
                as::buffer(buf_.data(), buf_.size())
            };
    }

    /**
     * @brief Copy the internal information to the range between b and e
     *        it is for boost asio APIs
     * @param b begin of the range to fill
     * @param e end of the range to fill
     */
    template <typename It>
    void fill(It b, It e) const {
        BOOST_ASSERT(std::distance(b, e) == size());
        *b++ = id_;
        std::copy(buf_.data(), buf_.data() + buf_.size(), b);
    }

    /**
     * @brief Get whole size of sequence
     * @return whole size
     */
    std::size_t size() const {
        return 1 + buf_.size();
    }

    char const id_;
    boost::container::static_vector<char, N> buf_;
};

struct variable_length_property {
    variable_length_property(std::size_t id, std::size_t size)
        :id_(id) {
        if (size > 0xfffffff) throw variable_length_error();
        while (size > 127) {
            buf_.emplace_back((size & 0b01111111) | 0b10000000);
            size >>= 7;
        }
        buf_.emplace_back(size & 0b01111111);
    }

    /**
     * @brief Create const buffer sequence
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        return
            {
                as::buffer(&id_, 1),
                as::buffer(buf_.data(), buf_.size())
            };
    }

    /**
     * @brief Copy the internal information to the range between b and e
     *        it is for boost asio APIs
     * @param b begin of the range to fill
     * @param e end of the range to fill
     */
    template <typename It>
    void fill(It b, It e) const {
        BOOST_ASSERT(std::distance(b, e) == size());
        *b++ = id_;
        std::copy(buf_.data(), buf_.data() + buf_.size(), b);
    }

    /**
     * @brief Get whole size of sequence
     * @return whole size
     */
    std::size_t size() const {
        return 1 + buf_.size();
    }

    char const id_;
    boost::container::static_vector<char, 4> buf_;
};

} // namespace detail

class payload_format_indicator : public detail::n_bytes_property<1> {
public:
    payload_format_indicator(bool binary = true)
        : detail::n_bytes_property<1>(0x01, binary ? 0 : 1) {}
};

class user_property {
    struct len_str {
        boost::container::static_vector<char, 2> len;
        as::const_buffer str;
    };
    struct entry {
        len_str key;
        len_str val;
    };
public:
    user_property() {}

    /**
     * @brief Create const buffer sequence
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        std::vector<as::const_buffer> ret;
        ret.reserve(
            1 +
            entries_.size() * 2
        );

        ret.emplace_back(as::buffer(&id_, 1));

        for (auto const& e : entries_) {
            ret.emplace_back(as::buffer(e.first.len.data(), e.first.len.size()));
            ret.emplace_back(e.first.str);
            ret.emplace_back(as::buffer(e.second.len.data(), e.second.len.size()));
            ret.emplace_back(e.second.str);
        }

        return ret;
    }

    template <typename It>
    void fill(It b, It e) const {
        BOOST_ASSERT(std::distance(b, e) == size());

        *b++ = id_;
        for (auto const& e : entries_) {
            {
                std::copy(e.first.len.begin(), e.first.len.end(), b);
                b += e.first.len.size();
                auto ptr = get_pointer(e.first.str);
                auto size = get_size(e.first.str);
                std::copy(ptr, ptr + size, b);
                b += size;
            }
            {
                std::copy(e.second.len.begin(), e.second.len.end(), b);
                b += e.second.len.size();
                auto ptr = get_pointer(e.second.str);
                auto size = get_size(e.second.str);
                std::copy(ptr, ptr + size, b);
                b += size;
            }
        }
    }

    /**
     * @brief Get whole size of sequence
     * @return whole size
     */
    std::size_t size() const {
        return
            1 + // id_
            entries_.size() * 2 * 2 + // pair of 2 bytes(size)
            [this] {
                std::size_t size = 0;
                for (auto const& e : entries_) {
                    size += get_size(e.first.str);
                    size += get_size(e.second.str);
                }
                return size;
            }();
    }

private:
    char const id_ = 0x26;
    std::vector<std::pair<len_str, len_str>> entries_;
};

} // namespace property
} // namespace v5
} // namespace mqtt

#endif // MQTT_PROPERTY_HPP
