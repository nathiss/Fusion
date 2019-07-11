/**
 * @file http_session.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the HTTPSession class.
 *
 * (c) 2019 by Kamil Rusin
 */

#pragma once

#include <cstdlib>

#include <memory>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <spdlog/spdlog.h>

namespace fusion_server {

/**
 * This class represents the HTTP session between a client and the server.
 */
class HTTPSession : public std::enable_shared_from_this<HTTPSession> {
 public:
  /**
   * This is the type of a HTTP request used in this class.
   */
  using Request_t = boost::beast::http::request<boost::beast::http::string_body>;

  /**
   * This is the type of a HTTP response used in this class.
   */
  using Response_t = boost::beast::http::response<boost::beast::http::string_body>;

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
   * This method calls asynchronous writing to the client.
   *
   * @param[in] response
   *   A HTTP response to be sent.
   *
   * @note
   *   This method takes the ownership of the response object.
   */
  void PerformAsyncWrite(Response_t response) noexcept;

  /**
   * @brief Constructs a response to the stored request.
   * This method returns a HTTP response to the stored HTTP request.
   *
   * @return
   *   A HTTP response to the stored request.
   */
  Response_t MakeResponse() const noexcept;

  /**
   * @brief Constructs a "Bad Request" (400) response.
   * This method returns a HTTP response with the status code set to 400.
   *
   * @return
   *   A HTTP "Bad Request" (400) response.
   *
   * @see [[RFC 2616] 10.4.1 400 Bad Request](https://tools.ietf.org/html/rfc2616#section-10.4.1)
   */
  Response_t MakeBadRequest() const noexcept;

  /**
   * @brief Checks if the error indicates a bad request.
   * This method returns an indication whether or not the given error code
   * indicates that a request is ill-formed.
   *
   * @param ec
   *   A Boost error code.
   *
   * @return
   *   An indication whether or not the given error code indicates that a
   *   request is ill-formed is returned.
   *
   * @see [boost::system::error_code](https://www.boost.org/doc/libs/1_67_0/libs/system/doc/reference.html#Class-error_code)
   */
  inline bool IsBadRequestError(const boost::system::error_code& ec) const noexcept;

  /**
   * @brief Checks if the error indicates a too large request.
   * This method returns an indication whether or not the given error code
   * indicates that a request (or some part of the request) is too large and
   * would cause buffer overflow.
   *
   * @param ec
   *   A Boost error code.
   *
   * @return
   *   An indication whether or not the given error code indicates that a
   *   request (or some part of the request) is too large and would cause buffer
   *   overflow is returned.
   *
   * @see [boost::system::error_code](https://www.boost.org/doc/libs/1_67_0/libs/system/doc/reference.html#Class-error_code)
   */
  inline bool IsTooLargeRequestError(const boost::system::error_code& ec) const noexcept;

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

  /**
   * @brief HTTPSession's logger.
   * This is a pointer to the logger used in HTTPSession class.
   */
  std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace fusion_server