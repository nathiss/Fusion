#pragma once

#include <cstdlib>

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <fusion_server/system_abstractions.hpp>

using fusion_server::system_abstractions::Package;

namespace fusion_server {

/**
 * This class represents the WebSocket session between a client and the server.
 */
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
 public:
  WebSocketSession(const WebSocketSession&) = delete;
  WebSocketSession(WebSocketSession&&) = delete;
  WebSocketSession& operator=(const WebSocketSession&) = delete;
  WebSocketSession& operator=(WebSocketSession&&) = delete;

  /**
   * This constructor takes the overship of the socket connected to a client and
   * registers this session to the server.
   *
   * @param[in] socket
   *   The socket connected to a client.
   */
  WebSocketSession(boost::asio::ip::tcp::socket socket) noexcept;

  /**
   * This destructor unregisters this session from the server.
   */
  ~WebSocketSession() noexcept;

  /**
   * This method delegates the write operation to the client.
   * Since Boost::Beast allows only one writing at a time, the packages is
   * always queued and if no writing is taking place the asynchronous write
   * operation is called.
   *
   * @param[in] package
   *   The package to be send to the client. The shared_ptr is used to ensure
   *   that the package will be freed only if it has been sent to all the
   *   specific clients.
   *
   * @note
   *   This method is thread-safe.
   */
  void Write(Package package) noexcept;

  /**
   * This method upgrades the connection to the WebSocket Protocol and performs
   * the asynchronous handshake.
   *
   * @param[in] request
   *   A HTTP Upgrade request.
   */
  template <typename Body, typename Allocator>
  void Run(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> request) noexcept;

  /**
   * This method closes the connection immediately. Any asynchronous operation
   * will be cancelled.
   */
  void Close() noexcept;

  /**
   * This method returns a value that indicates whether or not the socket is
   * connected to a client.
   *
   * @return
   *   A value that indicates whether or not the socket is connected to a client
   *   is returned.
   */
  explicit operator bool() const noexcept;

  /**
   * This method is the callback to asynchronous handshake with the client.
   *
   * @param[in] ec
   *   This is the Boost error code.
   */
  void HandleHandshake(const boost::system::error_code& ec) noexcept;

  /**
   * This method is the callback to asynchronous read from the client.
   *
   * @param[in] ec
   *   This is the Boost error code.
   *
   * @param[in] bytes_transmitted
   *   This is the amount of transmitted bytes.
   */
  void HandleRead(const boost::system::error_code& ec, std::size_t bytes_transmitted) noexcept;

  /**
   * This method is the callback to asynchronous write to the client.
   *
   * @param[in] ec
   *   This is the Boost error code.
   *
   * @param[in] bytes_transmitted
   *   This is the amount of transmitted bytes.
   */
  void HandleWrite(const boost::system::error_code& ec, std::size_t bytes_transmitted) noexcept;

 private:

  /**
   * This is the WebSocket wrapper around the socket connected to a client.
   */
  boost::beast::websocket::stream<boost::asio::ip::tcp::socket> websocket_;

  /**
   * This is the buffer for the incomming packages.
   */
  boost::beast::multi_buffer buffer_;

  /**
   * This is the strand for this instance of WebSocketSession class.
   */
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;

  /**
   * This queue holds all outgoing packages, which have not yet been sent.
   */
  std::queue<Package> outgoing_queue_;

  /**
   * This is the mutex for outgoing queue.
   */
  std::mutex outgoing_queue_mtx_;

  /**
   * This indicates whether or not the handshake has been completed.
   */
  std::atomic<bool> handshake_complete_;

  /**
   * This delegate is called each time when a new package arrives.
   */
  system_abstractions::IncommingPackageDelegate delegate_;
};

template <typename Body, typename Allocator>
void WebSocketSession::Run(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> request) noexcept {
  websocket_.async_accept(
    std::move(request),
    boost::asio::bind_executor(
      strand_,
      [self = shared_from_this()](
        const boost::system::error_code& ec
      ) {
        self->HandleHandshake(ec);
      }
    )
  );
}

}  // namespace fusion_server