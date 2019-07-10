#include <cstdlib>

#ifdef DEBUG
#include <iostream>
#endif
#include <memory>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <fusion_server/http_session.hpp>
#include <fusion_server/websocket_session.hpp>

namespace fusion_server {

HTTPSession::HTTPSession(boost::asio::ip::tcp::socket socket) noexcept
    : socket_{std::move(socket)}, strand_{socket_.get_executor()} {
}

HTTPSession::~HTTPSession() noexcept = default;

void HTTPSession::Run() noexcept {
  boost::beast::http::async_read(
    socket_,
    buffer_,
    request_,
    [self = shared_from_this()](
      const boost::system::error_code& ec,
      std::size_t bytes_transmitted
    ) {
      self->HandleRead(ec, bytes_transmitted);
    }
  );
}

void HTTPSession::Close() noexcept {
#ifdef DEBUG
  std::cout << "[HTTPSession: " << this << "] Closing" << std::endl;
#endif
  try {
    socket_.close();
  }
  catch (const boost::system::system_error& e) {
    std::cerr << "HTTPSession::Close: " << e.what() << std::endl;
    // We do nothing, because
    // https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload1.html
    // [Thrown on failure. Note that, even if the function indicates an error, the underlying descriptor is closed.]
  }
}

HTTPSession::operator bool() const noexcept {
  return socket_.is_open();
}

void HTTPSession::HandleRead(const boost::system::error_code& ec,
  [[ maybe_unused ]] std::size_t bytes_transmitted) noexcept {
#ifdef DEBUG
  std::cout << "[HTTPSession: " << this << "] Read " << bytes_transmitted << " bytes." << std::endl;
#endif
  if (ec == boost::beast::http::error::end_of_stream ||
    ec == boost::beast::http::error::partial_message) {
    // The client closed the connection. We either read none or read incomplete
    // message. We don't care about both.
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
    return;
  }
  if (ec == boost::beast::http::error::buffer_overflow ||
    ec == boost::beast::http::error::header_limit ||
    ec == boost::beast::http::error::body_limit) {
    // This means a HTTP content would exceed the maximum size of the buffer.
    // This should never happened in normal use, so we close the connection.
    std::cerr << "[HTTPSession: " << this << "] message from " << socket_.remote_endpoint()
      << " is too large. Closing the connection." << std::endl;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    return;
  }

  if (ec == boost::beast::http::error::bad_line_ending ||
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
    ec == boost::beast::http::error::bad_obs_fold) {
    // This means the request is ill-formed.
    PerformAsyncWrite(MakeBadRequest());
    }

  if (ec) {
    std::cerr << "HTTPSession::HandleRead: " << ec.message() << std::endl;
    return;
  }

  if (boost::beast::websocket::is_upgrade(request_)) {
#ifdef DEBUG
    std::cout << "[HTTPSession: " << this << "] Received an upgrade request." << std::endl;
#endif
    std::make_shared<WebSocketSession>(std::move(socket_))->Run(
      std::move(request_)
    );
    return;
  }

  PerformAsyncWrite(MakeResponse());
}

void HTTPSession::HandleWrite(const boost::system::error_code& ec,
  [[ maybe_unused ]] std::size_t bytes_transmitted, bool close) noexcept {
#ifdef DEBUG
  std::cout << "[HTTPSession: " << this << "] Written " << bytes_transmitted << " bytes." << std::endl;
#endif
  if (ec) {
    std::cerr << "HTTPSession::HandleWrite: " << ec.message() << std::endl;
    return;
  }

  if (close) {
    // The client closed its connection. We do the same.
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
    return;
  }

  // Clear contents of the request message, otherwise the read behavior is undefined.
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
    }
  );
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
    }
  );
}

auto HTTPSession::MakeResponse() const noexcept -> Response_t {
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

auto HTTPSession::MakeBadRequest() const noexcept -> Response_t {
  Response_t res{
    boost::beast::http::status::bad_request,
    request_.version()
  };
  res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(boost::beast::http::field::content_type, "text/html; charset=utf-8");
  res.keep_alive(request_.keep_alive());
  res.body() = "<html><body><h1>400 Bad Request</h1></body></html>";
  res.prepare_payload();
  return res;
}

}  // namespace fusion_server