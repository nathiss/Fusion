#pragma once

#include <memory>

#include <boost/asio.hpp>

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
   * This constructor takes the overship of the socket connected to a client.
   */
  WebSocketSession(boost::asio::ip::tcp::socket socket) noexcept;

  /**
   * This is the method for delegating the write operation to the client.
   * Since Boost::Beast allows only one writing at a time, the packages is
   * always queued and if no writing takes place it is delegated.
   * The method is thread-safe.
   *
   * @param[in] package
   *   The package to be send to the client. The shared_ptr is used to ensure
   *   that the package will be freed only if it has been sent to all the
   *   specific clients.
   */
  void Write(std::shared_ptr<const std::string> package) noexcept;

  /**
   * This method upgrades the connection to the WebSocket Protocol and performs
   * the async handshake.
   *
   * @param[in] request
   *   The Upgrade request.
   */
  template <typename Request>
  void Run(Request&& request) noexcept;

  /**
   * This method returns the oldest package sent by the client. If no package
   * has yet arrived, the nullptr is returned.
   * The method is thread-safe.
   *
   * @return
   *   The oldest package sent by the client is returned. If no package has yet
   *   arrived, the nullptr is returned.
   */
  std::shared_ptr<const std::string> Pop() noexcept;

  /**
   * This method closes the connection immediately. Any async operation will be
   * cancelled.
   */
  void Close() noexcept;

  /**
   * This method returns a value that indicates whether or not the socket is
   * connected to a client.
   *
   * @return
   *   A value that indicates whether or not the socket is
   * connected to a client is returned.
   */
  explicit operator bool() const noexcept;

 private:
  /**
   * This method is the callback to async handshaking with the client.
   *
   * @param[in] ec
   *   This is the Boost error code.
   */
  void HandleHandshake(const boost::system::error_code& ec) noexcept;

  /**
   * This method is the callback to async reading from the client.
   *
   * @param[in] ec
   *   This is the Boost error code.
   *
   * @param[in] bytes_transmitted
   *   This is the amount of transmitted bytes.
   */
  void HandleRead(const boost::system::error_code& ec, std::size_t bytes_transmitted) noexcept;

  /**
   * This method is the callback to async writing to the client.
   *
   * @param[in] ec
   *   This is the Boost error code.
   *
   * @param[in] bytes_transmitted
   *   This is the amount of transmitted bytes.
   */
  void HandleWrite(const boost::system::error_code& ec, std::size_t bytes_transmitted) noexcept;

  /**
   * This is the type of structure that contains the private
   * properties of the instance.  It is defined in the implementation
   * and declared here to ensure that it is scoped inside the class.
   */
  struct Impl;

  /**
   * This contains the private properties of the instance.
   */
  std::unique_ptr<Impl> impl_;
};

}  // namespace fusion_server