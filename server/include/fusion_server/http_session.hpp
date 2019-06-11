#pragma once

#include <memory>

#include <boost/asio.hpp>

namespace fusion_server {

/**
 * This class represents the HTTP session between a client and the server.
 */
class HTTPSession : public std::enable_shared_from_this<HTTPSession> {
 public:
  HTTPSession(const HTTPSession&) = delete;
  HTTPSession(HTTPSession&&) = delete;
  HTTPSession& operator=(const HTTPSession&) = delete;
  HTTPSession& operator=(HTTPSession&&) = delete;

  /**
   * This constructor takes the overship of the socket connected to a client.
   */
  HTTPSession(boost::asio::ip::tcp::socket socket) noexcept;

  /**
   * This method starts the loop of async reads. It is indended to be called
   * only once. If it is called more than once the behaviour is undefined.
   */
  void Run() noexcept ;

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
   * This method is a callback to async reading from the client. After parsing
   * the request it performs async responding.
   * 
   * @param[in] ec
   *   This is the Boost error code.
   * 
   * @param[in] bytes_transmitted
   *   This is the amount of transmitted bytes.
   */
  void HandleRead(const boost::system::error_code& ec, std::size_t bytes_transmitted) noexcept;

  /**
   * This method is a callback to async writing to the client. After checking if
   * an error occured it performs async reading from the client.
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