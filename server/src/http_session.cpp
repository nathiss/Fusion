#include <functional>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <fusion_server/http_session.hpp>

namespace fusion_server {

struct HTTPSession::Impl {
  // Methods
  Impl(boost::asio::ip::tcp::socket socket) noexcept
      : socket_{std::move(socket)}, strand_{socket_.get_executor()} {}

  // Atributes

  /**
   * This is the socket connected to the client.
   */
  boost::asio::ip::tcp::socket socket_;

  /**
   * This is the strand for this instance of the HTTPSession class.
   */
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;

  /**
   * This is the buffer used to store the client's requests.
   */
  boost::beast::flat_buffer buffer_;

  /**
   * This holds the parsed client's request.
   */
  boost::beast::http::request<boost::beast::http::string_body> request_;
};

HTTPSession::HTTPSession(boost::asio::ip::tcp::socket socket) noexcept
    : impl_{new Impl{std::move(socket)}} {
}

HTTPSession::~HTTPSession() noexcept = default;

void HTTPSession::Run() noexcept {
  boost::beast::http::async_read(
    impl_->socket_,
    impl_->buffer_,
    impl_->request_,
    std::bind(
      &HTTPSession::HandleRead,
      shared_from_this(),
      std::placeholders::_1,
      std::placeholders::_2
    )
  );
}

void HTTPSession::Close() noexcept {
  try {
    impl_->socket_.close();
  }
  catch (const boost::system::system_error& e) {
    std::cerr << "HTTPSession::Close: " << e.what() << std::endl;
    // We do nothing, because
    // https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload1.html
    // [Thrown on failure. Note that, even if the function indicates an error, the underlying descriptor is closed.]
  }
}

HTTPSession::operator bool() const noexcept {
  return impl_->socket_.is_open();
}

void HTTPSession::HandleRead(const boost::system::error_code& ec,
  [[ maybe_unused ]] std::size_t bytes_transmitted) noexcept {
  if (ec == boost::beast::http::error::end_of_stream) {
    // The client closed the connection.
    impl_->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
    return;
  }

  if (ec) {
    std::cerr << "HTTPSession::HandleRead: " << ec.message() << std::endl;
    return;
  }

  if (boost::beast::websocket::is_upgrade(impl_->request_)) {
    // TODO: create shared websocket session.
    return;
  }

  auto response = [&req_ = impl_->request_]{
    namespace http =  boost::beast::http;
    http::response<http::string_body> res{http::status::ok, req_.version()};
    res.set(http::field::server, "server_name"); // TODO: set the real server name
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(req_.keep_alive());
    res.body() = "FeelsBadMan\r\n";
    res.prepare_payload();
    return std::make_shared<decltype(res)>(std::move(res));
  }();

  boost::beast::http::async_write(
    impl_->socket_,
    *response,
    [self = shared_from_this(), response](
      const boost::system::error_code& ec,
      std::size_t bytes_transmitted
    ) {
      self->HandleWrite(ec, bytes_transmitted, response->need_eof());
    }
  );
}

void HTTPSession::HandleWrite(const boost::system::error_code& ec,
  [[ maybe_unused ]] std::size_t bytes_transmitted, bool close) noexcept {
  if (ec) {
    std::cerr << "HTTPSession::HandleWrite: " << ec.message() << std::endl;
    return;
  }

  if (close) {
    // The client closed its connection. We do the same.
    impl_->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
    return;
  }

  // Clear contents of the request message, otherwise the read behavior is undefined.
  impl_->request_ = {};

  boost::beast::http::async_read(
    impl_->socket_,
    impl_->buffer_,
    impl_->request_,
    std::bind(
      &HTTPSession::HandleRead,
      shared_from_this(),
      std::placeholders::_1,
      std::placeholders::_2
    )
  );
}

}  // namespace fusion_server