#pragma once

#include <memory>
#include <string>

namespace fusion_server {

/**
 * This is the interface for the connection between a client and the server.
 */
class ISession {
 public:
  /**
   * This is the default destructor.
   */
  virtual ~ISession() noexcept = default;

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
  virtual void Write(std::shared_ptr<const std::string> package) noexcept = 0;

  /**
   * This method starts the loop of async reads. It is indended to be called
   * only once. If it is called more than once the behaviour is undefined.
   */
  virtual void Run() noexcept = 0;

  /**
   * This method returns the oldest package sent by the client. If no package
   * has yet arrived, the nullptr is returned.
   * The method is thread-safe.
   * 
   * @return
   *   The oldest package sent by the client is returned. If no package has yet
   *   arrived, the nullptr is returned.
   */
  virtual std::shared_ptr<const std::string> Pop() noexcept = 0;

  /**
   * This method closes the connection immediately. Any async operation will be
   * cancelled.
   */
  virtual void Close() noexcept = 0;

  /**
   * This method returns a value that indicates whether or not the socket is
   * connected to a client.
   * 
   * @return
   *   A value that indicates whether or not the socket is
   * connected to a client is returned.
   */
  virtual explicit operator bool() const noexcept = 0;
};

}  // namespace fusion_server