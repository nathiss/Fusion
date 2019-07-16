/**
 * @file listener.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the Listener class.
 *
 * Copyright 2019 Kamil Rusin
 */

#pragma once

#include <cstdint>

#include <memory>
#include <string_view>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <fusion_server/logger_manager.hpp>

namespace fusion_server {

/**
 * This class represents the local endpoint used to accept new connections
 * from the clients.
 */
class Listener : public std::enable_shared_from_this<Listener> {
 public:
  /**
   * @brief Explicitly deleted copy constructor.
   * It's deleted due to presence of boost::asio's socket.
   *
   * @param[in] other
   *   Copied object.
   */
  Listener(const Listener& other) = delete;

  /**
   * @brief Explicitly deleted move constructor.
   * It's deleted just because it's not used. Although it could be dangerous,
   * due to asynchronous operations are made on this object.
   *
   * @param[in] other
   *   Moved object.
   */
  Listener(Listener&& other) = delete;

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
  Listener& operator=(const Listener& other) = delete;

  /**
   * @brief Explicitly deleted move operator.
   * It's deleted just because it's not used. Although it could be dangerous,
   * due to asynchronous operations are made on this object.
   *
   * @param[in] other
   *   Moved object.
   *
   * @return
   *   Reference to `this` object.
   */
  Listener& operator=(Listener&& other) = delete;

  /**
   * @brief Sets the used I/O context object.
   * This constructor sets the used I/O context to the given one.
   *
   * @param ioc [in]
   *   Reference to the I/O context used in all asynchronous operations.
   */
  explicit Listener(boost::asio::io_context& ioc) noexcept;

  /**
   * @brief Configures the listener.
   * This method configures the listener using the given JSON object.
   * If it returns false, a unrecoverable error occurred and the program should
   * exit immediately.
   *
   * @param config
   *   A JSON object containing configuration for the listener.
   *
   * @return
   *   An indication, whether or not the operation was successful is returned.
   */
  bool Configure(const json::JSON& config) noexcept;

  /**
   * @brief Sets the logger of this instance.
   * This method sets the logger of this instance to the given one.
   *
   * @param logger [in]
   *   The given logger.
   */
  void SetLogger(LoggerManager::Logger logger) noexcept;

  /**
   * @brief Returns this instance's logger.
   * This method returns the logger of this instance.
   *
   * @return
   *   The logger of this instance is returned. If the logger has not been set
   *   this method returns std::nullptr.
   */
  [[nodiscard]] LoggerManager::Logger GetLogger() const noexcept;

  /**
   * @brief Binds the listener.
   * This method binds the listener to the endpoint configured by Configure
   * method.
   *
   * @return
   *   An indication whether or not the binding was successful.
   *
   * @note
   *   If this method is called before a successful call of Configure, the
   *   behaviour is undefined.
   */
  bool Bind() noexcept;

  /**
   * @brief Binds the Listener to the given endpoint.
   *
   * @param endpoint
   *   The endpoint to bind to.
   *
   * @return
   *   An indication whether or not the binding was successful.
   */
  bool Bind(boost::asio::ip::tcp::endpoint endpoint) noexcept;

  /**
   * @brief Binds the Listener to the given address and port.
   *
   * @param address_str
   *   The IP address of a local interface.
   *
   * @param port
   *   The port number.
   *
   * @return
   *   An indication whether or not the assignment was successful.
   */
  bool Bind(std::string_view address_str, std::uint16_t port) noexcept;

  /**
   * @brief Binds the Listener to all local interfaces and the given port.
   *
   * @param port
   *   The port number.
   *
   * @return
   *   An indication whether or not the binding was successful.
   */
  bool Bind(std::uint16_t port) noexcept;

  /**
   * @brief Returns endpoint.
   * This method returns the endpoint currently set in this object.
   *
   * @return
   *   The endpoint currently set in this object.
   *
   * @note
   *   If the endpoint has not been set, the returned endpoint is default
   *   constructed.
   */
  const boost::asio::ip::tcp::endpoint& GetEndpoint() const noexcept;

  /**
   * This method starts the asynchronous accepting loop on the specified
   * endpoint. It returns an indication whether or not the operation was
   * successful.
   *
   * @return true
   *   Started the asynchronous accepting loop.
   *
   * @return false
   *   A critical error occurred. Operation failed.
   *
   * @note
   *   If the acceptor binding has not been successful (@ref is_open_ is set to
   *   false), this method returns false.
   */
  bool Run() noexcept;

  /**
   * @brief Returns total number of connections.
   * This method returns the number of all connections accepted by this object.
   *
   * @return
   *   The number of all connections accepted by this object is returned.
   */
  std::size_t GetNumberOfConnections() const noexcept;

  /**
   * @brief Returns maximum number of pending connections.
   * This method returns maximum number of pending connections on this object.
   *
   * @return
   *   Maximum number of pending connections on this object is returned.
   *
   * @see
   *   [basic_stream_socket::max_listen_connections](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/basic_stream_socket/max_listen_connections.html)
   */
  std::size_t GetMaxQueuedConnections() const noexcept;

  /**
   * This is the callback to asynchronous accept of a new connection.
   *
   * @param ec [in]
   *   The Boost error code.
   *
   * @see [Boost 1.67 AcceptHandler](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/AcceptHandler.html)
   */
  void HandleAccept(const boost::system::error_code& ec) noexcept;

 private:

  /**
   * This method opens the acceptor & binds it to the endpoint.
   *
   * @return
   *   An indication whether or not the binding was successful.
   */
  bool InitAcceptor() noexcept;

  /**
   * The context for providing core I/O functionality.
   */
  boost::asio::io_context& ioc_;
  /**
   * This is the acceptor for accepting new connections.
   */
  boost::asio::ip::tcp::acceptor acceptor_;

  /**
   * This is the socket used to handle a new incoming connection.
   */
  boost::asio::ip::tcp::socket socket_;

  /**
   * This is the local endpoint on which new connections will be accepted.
   */
  boost::asio::ip::tcp::endpoint endpoint_;

  /**
   * This is an indication whether or not this listener has been properly bind
   * to the endpoint.
   */
  bool is_open_;

  /**
   * This value is a total number of all connection accepted by this object.
   */
  std::size_t number_of_connections_;

  /**
   * This value is a maximum number of queued incoming
   */
  std::size_t max_queued_connections_;

  /**
   * @brief Listener's logger.
   * This is a pointer to the logger used in Listener class.
   */
  LoggerManager::Logger logger_;

};

}  // namespace fusion_server
