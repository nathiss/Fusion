#pragma once

#include <cstdlib>

#include <memory>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace fusion_server {

/**
 * This class represents the HTTP session between a client and the server.
 */
class HTTPSession : public std::enable_shared_from_this<HTTPSession> {
 public:
  /**
   * @brief Explicitly deleted copy constructor.
   * It's deleted due to presence of boost::asio's socket.
   *
   * @param[in] other
   *   Copied object.
   */
  HTTPSession(const HTTPSession& other) = delete;

  /**
   * @brief Explitly deleted move constructor.
   * It's deleted just because it's not used. Although it could be dangerous,
   * due to asynchronous operations are made on this object.
   *
   * @param[in] other
   *   Moved object.
   */
  HTTPSession(HTTPSession&& other) = delete;

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
  HTTPSession& operator=(const HTTPSession& other) = delete;

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
  HTTPSession& operator=(HTTPSession&& other) = delete;

  /**
   * This constructor takes the overship of the socket connected to a client.
   *
   * @param[in] socket
   *   @brief A socket connected to a client.
   *   If the socket is not connected or is not in "ready" state, the behaviour
   *   is undefined.
   *
   */
  HTTPSession(boost::asio::ip::tcp::socket socket) noexcept;

  /**
   * This is the default destructor.
   */
  ~HTTPSession() noexcept;

  /**
   * This method starts the loop of asynchronous reads. It is indended to be
   * called only once. If it is called more than once the behaviour is
   * undefined.
   */
  void Run() noexcept;

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
   *   A value that indicates whether or not the socket is
   * connected to a client is returned.
   */
  explicit operator bool() const noexcept;

  /**
   * This method is the callback to asynchronous reading from the client.
   * After parsing the request it performs asynchronous responding.
   *
   * @param[in] ec
   *   This is the Boost error code.
   *
   * @param[in] bytes_transmitted
   *   This is the amount of transmitted bytes.
   */
  void HandleRead(const boost::system::error_code& ec, std::size_t bytes_transmitted) noexcept;

  /**
   * This method is the callback to asynchronous writing to the client.
   * After checking if an error occured it performs asynchronous reading from
   * the client.
   *
   * @param[in] ec
   *   This is the Boost error code.
   *
   * @param[in] bytes_transmitted
   *   This is the amount of transmitted bytes.
   *
   * @param[in] close
   *   This indicates whether or not we should close the connection.
   *   The response indicated the "Connection: close" semantic.
   */
  void HandleWrite(const boost::system::error_code& ec, std::size_t bytes_transmitted, bool close) noexcept;

 private:

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

}  // namespace fusion_server