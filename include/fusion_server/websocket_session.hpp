/**
 * @file websocket_session.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the WebSocketSession class.
 *
 * @see fusion_server::WebSocketSession
 *
 * (c) 2019 by Kamil Rusin
 */

#pragma once

#include <cstdlib>

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <spdlog/spdlog.h>

#include <fusion_server/system_abstractions.hpp>

using fusion_server::system_abstractions::Package;

namespace fusion_server {

/**
 * This class represents the WebSocket session between a client and the server.
 * Boost::Beast library is used to perform communication.
 *
 * @see [Boost::Beast](https://www.boost.org/doc/libs/1_67_0/libs/beast/doc/html/index.html)
 */
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
 public:
  /**
   * @brief Explicitly deleted copy constructor.
   * It's deleted due to presence of boost::asio's socket.
   *
   * @param[in] other
   *   Copied object.
   */
  WebSocketSession(const WebSocketSession& other) = delete;

  /**
   * @brief Explitly deleted move constructor.
   * It's deleted just because it's not used. Although it could be dangerous,
   * due to asynchronous operations are made on this object.
   *
   * @param[in] other
   *   Moved object.
   */
  WebSocketSession(WebSocketSession&& other) = delete;

  /**
   * @brief Explicitly deleted copy operator.
   * It's deleted due to presence of socket in class hierarchy.
   *
   * @param[in] other
   *   Copied object.
   *
   * @return
   *   Reference to `this` object.
   */
  WebSocketSession& operator=(const WebSocketSession& other) = delete;

  /**
   * @brief Explitly deleted move operator.
   * It's deleted just because it's not used. Although it could be dangerous,
   * due to asynchronous operations are made on this object.
   *
   * @param[in] other
   *   Moved object.
   *
   * @return
   *   Reference to `this` object.
   */
  WebSocketSession& operator=(WebSocketSession&& other) = delete;

  /**
   * This constructor takes the overship of the socket connected to a client and
   * registers this session to the server.
   *
   * @param[in] socket
   *   The socket connected to a client.
   *
   * @see [boost::asio::ip::tcp::socket](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/ip__tcp/socket.html)
   */
  explicit WebSocketSession(boost::asio::ip::tcp::socket socket) noexcept;

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
  void Write(const std::shared_ptr<Package>& package) noexcept;

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
   * This method allows the current writing to complete (if any) and then closes
   * the connection. After its called no writing should be performed.
   *
   * @note
   *   This method is thread safe. It is indended to be called only once.
   *   If it is called more than once the behaviour is undefined.
   */
  void Close() noexcept;

  /**
   * This method performs a writing to the client and, when its completed,
   * closes the connection. After its called no writing should be performed.
   *
   * @param[in] package
   *   The package to be send to the client.
   *
   * @note
   *   This method is thread safe. It is indended to be called only once.
   *   If it is called more than once the behaviour is undefined.
   */
  void Close(const std::shared_ptr<Package>& package) noexcept;

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
   * @brief Returns the remote endpoint.
   * This method returns a reference to the remote endpoint connected to the
   * stored socket.
   *
   * @return
   *   A reference to the remote endpoint connected to the stored socket is
   *   returned.
   *
   * @note
   *   This method has been created for debugging and logging only.
   */
  const boost::asio::ip::tcp::socket::endpoint_type&
  GetRemoteEndpoint() const noexcept;

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

  /**
   * This delegate is called each time when a new package arrives.
   */
  system_abstractions::IncommingPackageDelegate delegate_; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

 private:

  /**
   * This is the WebSocket wrapper around the socket connected to a client.
   */
  boost::beast::websocket::stream<boost::asio::ip::tcp::socket> websocket_;

  /**
   * @brief The remote endpoint.
   * This is the remote endpoint of the websocket_'s underlying socket.
   */
  decltype(websocket_)::next_layer_type::endpoint_type remote_endpoint_;

  /**
   * This is the buffer for the incoming packages.
   */
  boost::beast::multi_buffer buffer_;

  /**
   * This is the strand for this instance of WebSocketSession class.
   */
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;

  /**
   * This queue holds all outgoing packages, which have not yet been sent.
   */
  std::deque<std::shared_ptr<Package>> outgoing_queue_;

  /**
   * This is the mutex for outgoing queue.
   */
  std::mutex outgoing_queue_mtx_;

  /**
   * This indicates whether or not the handshake has been completed.
   */
  std::atomic<bool> handshake_complete_;

  /**
   * This indicates wheter or not the closing procedure has started.
   * If it's true, no writing to the
   */
  std::atomic<bool> in_closing_procedure_;

  /**
   * @brief WebSocketSession's logger.
   * This is a pointer to the logger used in WebSocketSession class.
   */
  std::shared_ptr<spdlog::logger> logger_;
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
