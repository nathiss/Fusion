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

auto Server::Register(WebSocketSession* new_session) noexcept
    -> system_abstractions::IncommingPackageDelegate& {
  std::lock_guard l{unidentified_sessions_mtx_};
  auto [it, took_place] = unidentified_sessions_.insert(new_session);

  if (!took_place) {
    std::cerr << "Server::Register: the session " << new_session
    << " has already been registered." << std::endl;
  }

  return unjoined_delegate_;
}

void Server::Unregister(WebSocketSession* session) noexcept {
  if (std::lock_guard l{unidentified_sessions_mtx_}; unidentified_sessions_.count(session) > 0) {
    unidentified_sessions_.erase(session);
  }

}

void Server::StartAccepting() noexcept {
  std::make_shared<Listener>(ioc_, "127.0.0.1", 8080)->Run();
}

Server::Server() noexcept {
  unjoined_delegate_ = [this](
    std::shared_ptr<const std::string> package, WebSocketSession* src) {
    auto parsed = package_parser_.Parse(*package);
    if (!parsed) {
      PackageParser::JSON j;
      j["error_message"] = "Package does not contain a valid JSON.";
      src->Write(std::make_shared<const std::string>(j.dump()));
      src->Close();
    }
    src->Write(package);
  };
}

}  // namespace fusion_server