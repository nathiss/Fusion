#pragma once

#include <mutex>
#include <set>

#include <boost/asio.hpp>

#include <fusion_server/listener.hpp>

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
   *
   * @param[in] new_session
   *   A new session to be registered.
   *
   * @note
   *   This method is thread-safe.
   */
  void Register(WebSocketSession* new_session) noexcept;

  /**
   * This method unregisters the given session. After that method is executed,
   * all shared pointers of that session should go out of scope and the object
   * itself should be destructed.
   *
   * @param[in] session
   *   The session to be unregistered.
   *
   * @note
   *   This method is thread-safe. If it's executed concurrently with the same
   *   argument, the second call will do nothing
   */
  void Unregister(WebSocketSession* session) noexcept;

  /**
   * This method binds the listener to the interface taken from the
   * configuration file and runs the accepting loop.
   */
  void StartAccepting() noexcept;

 private:

  /**
   * This is the pointer pointing to the only instance of this class.
   */
  static std::unique_ptr<Server> instance_;

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
};

}  // namespace fusion_server