#pragma once

#include <boost/asio.hpp>
#include <spdlog/fmt/ostr.h>

namespace fusion_server {

/**
 * @brief Prints endpoints.
 * This function is used to print endpoints into logger messages. It returns a
 * reference to @ref os argument.
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