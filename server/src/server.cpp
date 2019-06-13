#include <iostream>

#include <fusion_server/server.hpp>
#include <fusion_server/websocket_session.hpp>

namespace fusion_server {

std::unique_ptr<Server> Server::instance_ = nullptr;

Server& Server::GetInstance() noexcept {
  if (instance_ == nullptr) {
    instance_.reset(new Server);
  }
  return *instance_;
}

boost::asio::io_context& Server::GetIOContext() noexcept {
  return ioc_;
}

void Server::Register(WebSocketSession* new_session) noexcept {
  std::lock_guard l{unidentified_sessions_mtx_};
  auto [it, took_place] = unidentified_sessions_.insert(new_session);

  if (!took_place) {
    std::cerr << "Server::Register: the session " << new_session
    << " has already been registered." << std::endl;
  }
}

void Server::Unregister(WebSocketSession* session) noexcept {
  if (std::lock_guard l{unidentified_sessions_mtx_}; unidentified_sessions_.count(session) > 0) {
    unidentified_sessions_.erase(session);
  }

}

void Server::StartAccepting() noexcept {
  std::make_shared<Listener>(ioc_, "127.0.0.1", 8080)->Run();
}

}  // namespace fusion_server