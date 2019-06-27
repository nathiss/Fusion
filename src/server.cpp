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
#ifdef DEBUG
  std::cout << "[Server: " << this << "] New session registered: " << new_session << std::endl;
#endif

  if (!took_place) {
    std::cerr << "Server::Register: the session " << new_session
    << " has already been registered." << std::endl;
  }

  {
    std::lock_guard l{sessions_correlation_mtx_};
    sessions_correlation_[new_session] = {};
  }

  return unjoined_delegate_;
}

void Server::Unregister(WebSocketSession* session) noexcept {
  sessions_correlation_mtx_.lock();
  auto& game_name = sessions_correlation_[session];
  sessions_correlation_.erase(session);
  sessions_correlation_mtx_.unlock();

  if (!game_name) {
#ifdef DEBUG
  std::cout << "[Server: " << this << "] Removing session: " << session << std::endl;
#endif
    std::lock_guard l{unidentified_sessions_mtx_};
    unidentified_sessions_.erase(session);
    return;
  }

  std::lock_guard l{games_mtx_};
  auto& game = games_[game_name.value()];
  game.Leave(session);
  if (game.GetPlayersCount() == 0) {
#ifdef DEBUG
    std::cout << "[Server: " << this << "] Game " << &game << " has no more playes. Removing." << std::endl;
#endif
    games_.erase(game_name.value());
  }
}

void Server::StartAccepting() noexcept {
  std::make_shared<Listener>(ioc_, "127.0.0.1", 8080)->Run();
}

Server::Server() noexcept {
  unjoined_delegate_ = [this](
    Package package, WebSocketSession* src) {
#ifdef DEBUG
    std::cout << "[Server " << this << "] A new package from " << src << std::endl;
#endif
    auto parsed = package_parser_.Parse(*package);
    if (!parsed) {
      PackageParser::JSON response = {
        {"error_message", "Package does not contain a valid request."},
        {"closed", true}
      };
      src->Close(system_abstractions::make_Package(response.dump()));
      return;
    }
    auto request = std::move(parsed.value());
    // TODO: move it elsewhere
    if (request.find("game") != request.end() &&
        request.find("nick") != request.end() &&
        request.find("id")   != request.end() &&
        request.size() == 3) {
      bool joined{false};
      {
        std::lock_guard l{games_mtx_};
        auto& game = games_[request["game"]];
        joined = game.Join(src);
        if (joined) // TODO: do it really have to be here?
          src->delegate_ = game.GetDelegate();
      }
      if (!joined) {
        PackageParser::JSON response = {
          {"id", request["id"]},
          {"result", "full"}
        };
        src->Write(system_abstractions::make_Package(response.dump()));
        return;
      }

      unidentified_sessions_.erase(src);
      {
        std::lock_guard l{sessions_correlation_mtx_};
        sessions_correlation_[src] = request["game"];
      }

      PackageParser::JSON response = {
        {"id", request["id"]},
        {"result", "joined"},
        {"rays", decltype(response)::array()},
        {"players", decltype(response)::array(
          {
            {"player_id", 0},
            {"nick", request["nick"]}, // TODO: add check if the nick exists
            {"role", "none"},
            {"color", {255.0, 255.0, 255.0}},
            {"health", 100.0},
            {"position", {7.6, 67.2}},
            {"angle", 34.6},
          }
        )},
      };
      src->Write(system_abstractions::make_Package(response.dump()));
      return;

    } else { // It's not join request or it's ill-formed.
      PackageParser::JSON response = {
        {"error_message", "The package has not been recognised."},
        {"closed", true}
      };
      src->Close(system_abstractions::make_Package(response.dump()));
      return;
    }
  };
}

}  // namespace fusion_server