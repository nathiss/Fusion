/**
 * @file websocket_session.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the WebSocketSession class.
 *
 * (c) 2019 by Kamil Rusin
 */

#include <cstdlib>

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <fusion_server/logger_types.hpp>
#include <fusion_server/server.hpp>
#include <fusion_server/websocket_session.hpp>

namespace fusion_server {

WebSocketSession::WebSocketSession(boost::asio::ip::tcp::socket socket) noexcept
    : websocket_{std::move(socket)},
      remote_endpoint_{websocket_.next_layer().remote_endpoint()},
      strand_{websocket_.get_executor()},
      handshake_complete_{false},
      in_closing_procedure_{false} {
  logger_ = spdlog::get("websocket");
  delegate_ = Server::GetInstance().Register(this);
}

WebSocketSession::~WebSocketSession() noexcept {
  Server::GetInstance().Unregister(this);
};

void WebSocketSession::Write(Package package) noexcept {
  std::lock_guard l{outgoing_queue_mtx_};

  if (in_closing_procedure_) {
    logger_->warn("Trying to write to {} while in closing procedue.", GetRemoteEndpoint());
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
    logger_->warn("Trying to write to {} before handshake was complete.", GetRemoteEndpoint());
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
    logger_->warn("An error occured during closing a websocket. [Boost: {}]",
      ec.message());
    // TODO: Find out if the comment below applies to the WebSocket connections.
    // We do nothing, because
    // https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/basic_stream_socket/close/overload1.html
    // [Set on failure. Note that, even if the function indicates an error, the underlying descriptor is closed.]
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
    logger_->error("An error occured during sync writing the closing message. [Boost:{}]",
      ec.message());
  }
  Close();
}

WebSocketSession::operator bool() const noexcept {
  return websocket_.is_open();
}

const boost::asio::ip::tcp::socket::endpoint_type&
WebSocketSession::GetRemoteEndpoint() const noexcept {
  return remote_endpoint_;
}

void WebSocketSession::HandleHandshake(const boost::system::error_code& ec) noexcept {
  if (ec) {
    logger_->error("An error occured during handshake. Closing the session to {}. [Boost: {}]",
      GetRemoteEndpoint(), ec.message());
    return;
  }
  handshake_complete_ = true;

  logger_->debug("Handshake to {} completed.", GetRemoteEndpoint());

  if (std::lock_guard l{outgoing_queue_mtx_}; !outgoing_queue_.empty()) {
    logger_->debug("Sending a message queued before handshake completion.");
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

  logger_->debug("Read {} bytes from {}.", bytes_transmitted, GetRemoteEndpoint());
  // TODO: find out what's that doing.
  boost::ignore_unused(buffer_);

  if (ec == boost::beast::websocket::error::closed) {
    logger_->debug("The session to {} was closed.", GetRemoteEndpoint());
    return;
  }
  if (ec == boost::asio::error::operation_aborted) {
    logger_->debug("The read operation from {} was abroted.", GetRemoteEndpoint());
    return;
  }

  if (ec) {
    logger_->error("An error occured during reading from {}. [Boost: {}]",
      GetRemoteEndpoint(), ec.message());
    // We assume what the session cannot be fixed.
    // TODO: Find out if that's true.
    return;
  }

  const auto package = boost::beast::buffers_to_string(buffer_.data());
  buffer_.consume(buffer_.size());

  auto [is_valid, msg] = package_verifier_.Verify(std::move(package));

  if (!is_valid) {
    logger_->warn("A package from {} was not valid. Closing the connection.",
      GetRemoteEndpoint());
    Close(system_abstractions::make_Package(msg.dump()));
    return;
  }

  boost::asio::post(websocket_.get_executor(), [this, msg = std::move(msg)]{
    delegate_(std::move(msg), this);
  });

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
  logger_->debug("Written {} bytes to {}.", bytes_transmitted,
    GetRemoteEndpoint());

  if (ec == boost::beast::websocket::error::closed) {
    logger_->debug("The session to {} was closed.", GetRemoteEndpoint());
    return;
  }

  if (ec) {
    logger_->error("An error occured during writing to {}. [Boost: {}]",
      GetRemoteEndpoint(), ec.message());
    return;
  }

  std::lock_guard l{outgoing_queue_mtx_};
  outgoing_queue_.pop_front();

  if (in_closing_procedure_) {
    // We're in closing procedure. The next package is the last one to be sent.
    // After writing we close the session.
    logger_->debug("[ClosingProcedure] Sync writing the closing package to {}.",
      GetRemoteEndpoint());

    if (!outgoing_queue_.empty()) {
      boost::system::error_code ec;
      websocket_.write(boost::asio::buffer(*outgoing_queue_.front()), ec);

      if (ec) {
        logger_->error("An error occured during writing closing package to {}. [Boost: {}]",
          GetRemoteEndpoint(), ec.message());
      }
    }
    Close();
    return;
  }  // in_closing_procedure_

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