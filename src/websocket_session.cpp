#include <cstdlib>

#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <fusion_server/server.hpp>
#include <fusion_server/websocket_session.hpp>

namespace fusion_server {

WebSocketSession::WebSocketSession(boost::asio::ip::tcp::socket socket) noexcept
    : websocket_{std::move(socket)}, strand_{websocket_.get_executor()},
      handshake_complete_{false}, in_closing_procedure_{false} {
  delegate_ = Server::GetInstance().Register(this);
}

WebSocketSession::~WebSocketSession() noexcept {
  Server::GetInstance().Unregister(this);
};

void WebSocketSession::Write(Package package) noexcept {
  std::lock_guard l{outgoing_queue_mtx_};

  if (in_closing_procedure_) {
    // We're not allowed to queued any package, when the closing procedure has
    // started.
    return;
  }

  outgoing_queue_.push_back(package);

  if (outgoing_queue_.size() > 1) {
    // Means we're already writing.
    return;
  }

  if (!handshake_complete_) {
    // We need to wait for the handshake to complete.
    return;
  }

  websocket_.async_write(
      boost::asio::buffer(*outgoing_queue_.front()),
      boost::asio::bind_executor(
        strand_,
        [self = shared_from_this()](
          const boost::system::error_code& ec,
          std::size_t bytes_transmitted
        ) {
          self->HandleWrite(ec, bytes_transmitted);
        }
      )
  );
}

void WebSocketSession::Close() noexcept {
  if (!in_closing_procedure_) {
    std::lock_guard l{outgoing_queue_mtx_};
    in_closing_procedure_ = true;
    if (outgoing_queue_.size() > 1) {
      // This means there are queued additional packages. We return and allow
      // HandleWrite to call this method after the writing has been completed.
      outgoing_queue_.erase(outgoing_queue_.begin() + 1, outgoing_queue_.end());
      return;
    }
    if (outgoing_queue_.size() == 1) {
      // This means there are no additional packages. We just returnand allow
      // HandleWrite to call this method after the writing has been completed.
      return;
    }
  }

  // If we're here it means either there are no packages queued or we are
  // already in closing procedure.

  boost::system::error_code ec;
  websocket_.close(boost::beast::websocket::close_code::none, ec);
    // Now callers should not performs writing to the WebSocket and should
    // perform reading as long as a read returns boost::beast::websocket::closed
    // error.

  if (ec) {
    std::cerr << "WebSocketSession::Close: " << ec.message() << std::endl;
    // TODO: read if the comment below applies to the WebSocket connections.
    // We do nothing, because
    // https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload1.html
    // [Thrown on failure. Note that, even if the function indicates an error, the underlying descriptor is closed.]
  }
}

void WebSocketSession::Close(Package package) noexcept {
  // We lock the mutex first, to ensure that no additional package will be
  // queued, after the closing procedure has started.
  std::lock_guard l{outgoing_queue_mtx_};
  in_closing_procedure_ = true;

  if (outgoing_queue_.size() > 1) {
    // This means we're already writing and other packages are waiting.
    // We remove all additional packages and queue the closing package.
    outgoing_queue_.erase(outgoing_queue_.begin() + 1, outgoing_queue_.end());
    outgoing_queue_.push_back(package);
    return;
  }
  if (outgoing_queue_.size() == 1) {
    // This means we're already writing and no packages are waiting.
    // We queue the closing package and return. HandleWrite method will perform
    // sending and closing.
    outgoing_queue_.push_back(package);
    return;
  }

  // If we're here it means no writing is performed right now.
  boost::system::error_code ec;
  websocket_.write(boost::asio::buffer(*package), ec);
  if (ec) {
    std::cerr << "WebSocketSession::Close: " << ec.message() << std::endl;
    // Since we want to close the session and an error occured we just return
    // to destroy the object.
    return;
  }
  Close();
}

WebSocketSession::operator bool() const noexcept {
  return websocket_.is_open();
}

void WebSocketSession::HandleHandshake(const boost::system::error_code& ec) noexcept {
  if (ec) {
    std::cerr << "WebSocketSession::HandleHandshake: " << ec.message() << std::endl;
    // We assume what the session cannot be fixed.
    // TODO: Find out if that's true.
    return;
  }
  handshake_complete_ = true;

  if (std::lock_guard l{outgoing_queue_mtx_}; !outgoing_queue_.empty()) {
    websocket_.async_write(
      boost::asio::buffer(*outgoing_queue_.front()),
      boost::asio::bind_executor(
        strand_,
        [self = shared_from_this()](
          const boost::system::error_code& ec,
          std::size_t bytes_transmitted
        ) {
          self->HandleWrite(ec, bytes_transmitted);
        }
      )
    );
  }

  websocket_.async_read(
    buffer_,
    boost::asio::bind_executor(
      strand_,
      [self = shared_from_this()](
        const boost::system::error_code& ec,
        std::size_t bytes_transmitted
      ) {
        self->HandleRead(ec, bytes_transmitted);
      }
    )
  );
}

void WebSocketSession::HandleRead(const boost::system::error_code& ec,
  [[ maybe_unused ]] std::size_t bytes_transmitted) noexcept {
  // TODO: find out what's that doing.
  boost::ignore_unused(buffer_);

  if (ec == boost::beast::websocket::error::closed) {
    // The WebSocketSession was closed. We don't need to report that.
    return;
  }
  if (ec == boost::asio::error::operation_aborted) {
    // The operation has been canceled due to some server's inner operation
    // (e.g. WebSocketSession::Close() has been called).
    return;
  }
  if (ec) {
    std::cerr << "WebSocketSession::HandleRead: " << ec.message() << std::endl;
    // We assume what the session cannot be fixed.
    // TODO: Find out if that's true.
    return;
  }

  const auto package = boost::beast::buffers_to_string(buffer_.data());
  buffer_.consume(buffer_.size());

  delegate_(system_abstractions::make_Package(std::move(package)), this);

  websocket_.async_read(
    buffer_,
    boost::asio::bind_executor(
      strand_,
      [self = shared_from_this()](
        const boost::system::error_code& ec,
        std::size_t bytes_transmitted
      ) {
        self->HandleRead(ec, bytes_transmitted);
      }
    )
  );
}

void WebSocketSession::HandleWrite(const boost::system::error_code& ec,
  [[ maybe_unused ]] std::size_t bytes_transmitted) noexcept {
  if (ec == boost::beast::websocket::error::closed) {
    // The WebSocketSession was closed. We don't need to report that.
    return;
  }
  if (ec == boost::asio::error::operation_aborted) {
    // The operation has been canceled due to some server's inner operation
    // (e.g. WebSocketSession::Close() has been called).
    return;
  }
  if (ec) {
    std::cerr << "WebSocketSession::HandleWrite: " << ec.message() << std::endl;
    // We assume what the session cannot be fixed.
    // TODO: Find out if that's true.
    return;
  }

  std::lock_guard l{outgoing_queue_mtx_};
  outgoing_queue_.pop_front();

  if (in_closing_procedure_) {
    // We're in closing procedure. The next package is the last one to be sent.
    // After writing we close the session.
    if (!outgoing_queue_.empty()) {
      boost::system::error_code ec;
      websocket_.write(boost::asio::buffer(*outgoing_queue_.front()), ec);
      if (ec) {
        std::cerr << "WebSocketSession::HandleWrite: " << ec.message() << std::endl;
        // Since we want to close the session and an error occured we just
        // return to destroy the object.
        return;
      }
    }
    Close();
    return;
  }

  if (!outgoing_queue_.empty()) {
    websocket_.async_write(
      boost::asio::buffer(*outgoing_queue_.front()),
      boost::asio::bind_executor(
        strand_,
        [self = shared_from_this()](
          const boost::system::error_code& ec,
          std::size_t bytes_transmitted
        ) {
          self->HandleWrite(ec, bytes_transmitted);
        }
      )
    );
  }
}

}  // namespace fusion_server