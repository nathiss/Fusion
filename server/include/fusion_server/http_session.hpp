#pragma once

#include <memory>

#include <boost/asio.hpp>

#include <fusion_server/isession.hpp>

namespace fusion_server {

/**
 * This class represents the HTTP session between a client and the server.
 */
class HTTPSession : public ISession,
public std::enable_shared_from_this<HTTPSession> {
 public:
  HTTPSession(const HTTPSession&) = delete;
  HTTPSession(HTTPSession&&) = delete;
  HTTPSession& operator=(const HTTPSession&) = delete;
  HTTPSession& operator=(HTTPSession&&) = delete;

  /**
   * This is the destructor.
   */
  virtual ~HTTPSession() noexcept override;

  /**
   * This constructor takes the overship of the socket connected to a client.
   */
  HTTPSession(boost::asio::ip::tcp::socket socket) noexcept;

  /**
   * Since HTTP protocol is half-duplex, it's not possible to send a package to
   * a client independently. This method does nothing.
   */
  virtual void Write(std::shared_ptr<const std::string> package) noexcept override;

  virtual void Run() noexcept override;

  /**
   * No package from a client is queued. All requests are immediately responsed.
   * This method does nothing.
   * 
   * @return
   *   Always nullptr is returned.
   */
  virtual std::shared_ptr<const std::string> Pop() noexcept override;

  virtual void Close() noexcept override;

  virtual explicit operator bool() const noexcept override;

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