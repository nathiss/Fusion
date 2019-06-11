#pragma once

#include <memory>

#include <boost/asio.hpp>

#include "Iconnection.hpp"

namespace fusion_server {

/**
 * This class represents the connection between a client and the server.
 */
class Connection : public IConnection,
public std::enable_shared_from_this<Connection> {
 public:
  Connection(const Connection&) = delete;
  Connection(Connection&&) = delete;
  Connection& operator=(const Connection&) = delete;
  Connection& operator=(Connection&&) = delete;

  /**
   * This is the destructor.
   */
  virtual ~Connection() noexcept override;

  /**
   * This constructor takes the overship of the socket connected to a client.
   */
  Connection(boost::asio::ip::tcp::socket socket) noexcept;

  virtual void Write(std::shared_ptr<const std::string> package) noexcept override;

  virtual void Run() noexcept override;

  virtual std::shared_ptr<const std::string> Pop() noexcept override;

  virtual void Close() noexcept override;

  virtual explicit operator bool() const noexcept override;

 private:
  /**
   * This method is a callback to async handshaking with the client.
   * 
   * @param[in] ec
   *   This is the Boost error code.
   */
  void HandleHandshake(const boost::system::error_code& ec) noexcept;
  
  /**
   * This method is a callback to async reading from the client.
   * 
   * @param[in] ec
   *   This is the Boost error code.
   * 
   * @param[in] bytes_transmitted
   *   This is the amount of transmitted bytes.
   */
  void HandleRead(const boost::system::error_code& ec, std::size_t bytes_transmitted) noexcept;

  /**
   * This method is a callback to async writing to the client.
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