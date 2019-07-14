/**
 * @file http_session.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the HTTPSession class.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <cstdlib>

#include <memory>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <fusion_server/http_session.hpp>
#include <fusion_server/logger_types.hpp>
#include <fusion_server/websocket_session.hpp>

namespace fusion_server {

HTTPSession::HTTPSession(boost::asio::ip::tcp::socket socket) noexcept
    : socket_{std::move(socket)}, strand_{socket_.get_executor()} {
  logger_ = spdlog::get("http");
}

void HTTPSession::Run() noexcept {
  if (!(*this)) {
    return;
  }
  boost::beast::http::async_read(
    socket_,
    buffer_,
    request_,
    [self = shared_from_this()](
      const boost::system::error_code& ec,
      std::size_t bytes_transmitted
    ) {
      self->HandleRead(ec, bytes_transmitted);
  });
}

void HTTPSession::Close() noexcept {
  auto endpoint = socket_.remote_endpoint();
  logger_->debug("Closing connection to {}.", endpoint);

  boost::system::error_code ec;
  socket_.close(ec);

  if (ec) {
    logger_->warn("An error occurred during closing the connection to {}", endpoint);
    // We do nothing, because
    // https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload1.html
    // [Set on failure. Note that, even if the function indicates an error, the underlying descriptor is closed.]
  }
}

HTTPSession::operator bool() const noexcept {
  return socket_.is_open();
}

void HTTPSession::HandleRead(const boost::system::error_code& ec, std::size_t bytes_transmitted) noexcept {
  logger_->debug("Read {} bytes from {}.", bytes_transmitted,
    socket_.remote_endpoint());

  if (ec == boost::beast::http::error::end_of_stream ||
    ec == boost::beast::http::error::partial_message) {
    // Either client closed the connection or the connection timed out.
    logger_->debug("Connection from {} has been closed.", socket_.remote_endpoint());
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
    return;
  }
  if (IsTooLargeRequestError(ec)) {
    logger_->warn("A request from {} is too large. Closing the connection. [Size: {}]",
      socket_.remote_endpoint(), bytes_transmitted);
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    return;
  }

  if (IsBadRequestError(ec)) {
    PerformAsyncWrite(MakeBadRequest());
    return;
  }

  if (ec) {
    logger_->error("An error occurred during reading. [Boost:{}]", ec.message());
    return;
  }

  if (boost::beast::websocket::is_upgrade(request_)) {
    logger_->debug("Received an upgrade request from {}.", socket_.remote_endpoint());
    std::make_shared<WebSocketSession>(std::move(socket_))->Run(std::move(request_));
    return;
  }

  PerformAsyncWrite(MakeResponse());
}

void HTTPSession::HandleWrite(const boost::system::error_code& ec,
  std::size_t bytes_transmitted, bool close) noexcept {
  logger_->debug("Written {} bytes to {}.", bytes_transmitted,
    socket_.remote_endpoint());

  if (ec) {
    logger_->error("An error occurred during writing to {}. [Boost:{}]",
      socket_.remote_endpoint(), ec.message());
    return;
  }

  if (close) {
    // The client closed its connection. We do the same.
    logger_->debug("Client {} closed the connection. [KeepAlive: false]",
      socket_.remote_endpoint());
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
    return;
  }

  // Clear contents of the request message, otherwise the read behavior is undefined.
  buffer_.consume(buffer_.size());
  request_ = {};

  boost::beast::http::async_read(
    socket_,
    buffer_,
    request_,
    [self = shared_from_this()](
      const boost::system::error_code& ec,
      std::size_t bytes_transmitted
    ) {
      self->HandleRead(ec, bytes_transmitted);
  });
}

void HTTPSession::PerformAsyncWrite(Response_t response) noexcept {
  auto response_ptr = std::make_shared<decltype(response)>(std::move(response));

  boost::beast::http::async_write(
    socket_,
    *response_ptr,
    [self = shared_from_this(), response_ptr](
      const boost::system::error_code& ec,
      std::size_t bytes_transmitted
    ) {
      self->HandleWrite(ec, bytes_transmitted, response_ptr->need_eof());
  });
}

HTTPSession::Response_t HTTPSession::MakeResponse() const noexcept {
  Response_t res{
    request_.target() == "/" ? boost::beast::http::status::ok :
    boost::beast::http::status::not_found, request_.version()
  };
  res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(boost::beast::http::field::content_type, "text/plain; charset=utf-8");
  res.keep_alive(request_.keep_alive());
  res.body() = "FeelsBadMan\r\n";
  res.prepare_payload();
  return res;
}

HTTPSession::Response_t  HTTPSession::MakeBadRequest() const noexcept {
  Response_t res{
    boost::beast::http::status::bad_request,
    request_.version()
  };
  res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(boost::beast::http::field::content_type, "text/html; charset=utf-8");
  res.keep_alive(false);
  res.body() = "<html><body><h1>400 Bad Request</h1></body></html>";
  res.prepare_payload();
  return res;
}

bool HTTPSession::IsBadRequestError(const boost::system::error_code& ec) const noexcept {
  return ec == boost::beast::http::error::bad_line_ending ||
         ec == boost::beast::http::error::bad_method ||
         ec == boost::beast::http::error::bad_target ||
         ec == boost::beast::http::error::bad_version ||
         ec == boost::beast::http::error::bad_status ||
         ec == boost::beast::http::error::bad_reason ||
         ec == boost::beast::http::error::bad_field ||
         ec == boost::beast::http::error::bad_value ||
         ec == boost::beast::http::error::bad_content_length ||
         ec == boost::beast::http::error::bad_transfer_encoding ||
         ec == boost::beast::http::error::bad_chunk ||
         ec == boost::beast::http::error::bad_chunk_extension ||
         ec == boost::beast::http::error::bad_obs_fold;
}

bool HTTPSession::IsTooLargeRequestError(const boost::system::error_code& ec) const noexcept {
  return ec == boost::beast::http::error::buffer_overflow ||
         ec == boost::beast::http::error::header_limit ||
         ec == boost::beast::http::error::body_limit;
}

}  // namespace fusion_server
