/**
 * @file server.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the Server class.
 *
 * (c) 2019 by Kamil Rusin
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include <fusion_server/game.hpp>
#include <fusion_server/listener.hpp>
#include <fusion_server/system_abstractions.hpp>

namespace fusion_server {

/**
 * This is the forwadrd declaration of the WebSocketSession class.
 */
class WebSocketSession;

/**
 * This class represents the server itself. It holds all the WebSocket sessions
 * and manages all games.
 */
class Server {
 public:
  /**
   * This function returns the only instance of this class. If the instance has
   * not yet been created, the function creates it
   *
   * @return
   *   The only instance of this class is returned.
   */
  static Server& GetInstance() noexcept;

  /**
   * This method returns the reference to the I/O context used in this server.
   *
   * @return
   *   The reference to the I/O context used in this server is returned.
   */
  boost::asio::io_context& GetIOContext() noexcept;

  /**
   * This method adds the given session to the set of the unidentified sessions.
   * It returns the delegate to be called each time when a new package arrives.
   *
   * @param[in] new_session
   *   A new session to be registered.
   *
   * @note
   *   This method is thread-safe.
   *
   * @return
   *   The delegate to be called each time when a new package arrives is
   *   returned.
   */
  system_abstractions::IncommingPackageDelegate& Register(WebSocketSession* new_session) noexcept;

  /**
   * This method unregisters the given session. After that method is executed,
   * all shared pointers of that session should go out of scope and the object
   * itself should be destructed. If the given session is not registered in this
   * server, the method dose nothing.
   *
   * @param[in] session
   *   The session to be unregistered.
   *
   * @note
   *   This method is thread-safe. If it's executed concurrently with the same
   *   argument, the second call will do nothing.
   */
  void Unregister(WebSocketSession* session) noexcept;

  /**
   * This method creates a Listener object and calls Run() on it. It returns an
   * indication whether or not the opening of the acceptor was successful.
   *
   * @return
   *   An indication whether or not the opening of the acceptor was successful
   *   is returned.
   *
   * @note
   *   This method is indended to be called only once.
   *   If it is called more than once the behaviour is undefined.
   *
   * @see [class Listener](@ref Listener)
   */
  bool StartAccepting() noexcept;

  /**
   * @brief Closes all server's connections.
   * This method closes all connection stored in this server.
   *
   * @note
   *   It is indented to be called only once. If it's called more than once, the
   *   behaviour is undefined.
   */
  void Shutdown() noexcept;

 private:
  /**
   * This constructor is called only once, by the GetInstance() function.
   */
  Server() noexcept;

  /**
   * This method returns a response for the given request from a client.
   *
   * @param[in] session
   *   The WebSocket session connected to the client.
   *
   * @param[in] request
   *   This is a client's request.
   *
   * @return
   *   A response for the given request from a client is returned.
   */
  PackageParser::JSON
  MakeResponse(WebSocketSession* src, const PackageParser::JSON& request) noexcept;

  /**
   * This function object is called by WebSocket sessions from a clients, who
   * are not yet in any game, each time when a new package arrives.
   */
  system_abstractions::IncommingPackageDelegate unjoined_delegate_;

  /**
   * The context for providing core I/O functionality.
   */
  boost::asio::io_context ioc_;

  /**
   * This container holds all unidentifies WebSocket sessions.
   */
  std::set<WebSocketSession*> unidentified_sessions_;

  /**
   * This mutex is used to synchronise the access to the set of the unidentified
   * sessions.
   */
  std::mutex unidentified_sessions_mtx_;

  /**
   * This map associates all games in the server with their names.
   *
   * @note
   *   The shared_ptr is used to prevent calling copy constructor when creating
   *   `std::pair<...>` from an iterator.
   *   [Solution for this.](https://stackoverflow.com/a/10057516)
   */
  std::map<std::string, std::shared_ptr<Game>> games_;

  /**
   * This mutex is used to synchronise the access to the set of the games
   * collection.
   */
  std::mutex games_mtx_;

  /**
   * This map associates all sessions in the server and games to which they have
   * joined. If a sessions has not been joined to any game (it's unidentified)
   * the value is in its invalid state.
   */
  std::map<WebSocketSession*, std::optional<std::string>> sessions_correlation_;

  /**
   * This mutex is used to synchronise the access to the map of the sessions
   * and their games.
   */
  std::mutex sessions_correlation_mtx_;

  /**
   * @brief Server's logger.
   * This is a pointer to the logger used in Server class.
   */
  std::shared_ptr<spdlog::logger> logger_;

  /**
   * This flag indicates whether or not this server has stopped.
   *
   * @see fusion_server::Server#Shutdown
   */
  bool has_stopped_;

  /**
   * This is the pointer pointing to the only instance of this class.
   */
  static std::unique_ptr<Server> instance_;
};

}  // namespace fusion_server
