/**
 * @file logger_types.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares functions used to print "user types" by
 * [{fmt}](https://github.com/fmtlib/fmt) library.
 *
 * (c) 2019 by Kamil Rusin
 */

#pragma once

#include <boost/asio.hpp>
#include <spdlog/fmt/ostr.h>

namespace fusion_server {

/**
 * @brief Prints endpoints.
 * This function is used to print endpoints into logger messages. It returns a
 * reference to @p os argument.
 *
 * @tparam OStream
 *   A type which meets output stream concepts.
 *
 * @param os
 *   This is output stream used to print an endpoint.
 *
 * @param[in] endpoint
 *   This is an endpoint to be printed.
 *
 * @return
 *
 */
template <typename OStream>
OStream& operator<<(OStream& os, const boost::asio::ip::tcp::socket::endpoint_type& endpoint) noexcept {
  return os << endpoint;
}

}  // namespace fusion_server